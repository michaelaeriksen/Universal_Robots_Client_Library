/*
 * Copyright 2019, FZI Forschungszentrum Informatik (templating)
 *
 * Copyright 2017, 2018 Simon Rasmussen (refactor)
 *
 * Copyright 2015, 2016 Thomas Timm Andersen (original version)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <mutex>
#include <algorithm>

#include "readerwriterqueue/readerwriterqueue.h"
#include "ur_client_library/comm/package.h"
#include "ur_client_library/log.h"
#include "ur_client_library/helpers.h"

#undef ERROR

namespace urcl
{
namespace comm
{
/*!
 * \brief Parent class for for arbitrary consumers.
 *
 * @tparam T Type of the consumed products
 */
template <typename T>
class IConsumer
{
public:
  /*!
   * \brief Set-up functionality of the consumer.
   */
  virtual void setupConsumer()
  {
  }
  /*!
   * \brief Fully tears down the consumer - by default no difference to stopping it.
   */
  virtual void teardownConsumer()
  {
    stopConsumer();
  }
  /*!
   * \brief Stops the consumer.
   */
  virtual void stopConsumer()
  {
  }
  /*!
   * \brief Functionality for handling consumer timeouts.
   */
  virtual void onTimeout()
  {
  }

  /*!
   * \brief Consumes a product, utilizing it's contents.
   *
   * \param product Shared pointer to the product to be consumed.
   *
   * \returns Success of the consumption.
   */
  virtual bool consume(std::shared_ptr<T> product) = 0;
};

/*!
 * \brief Consumer, that allows one product to be consumed by multiple arbitrary
 * conusmers.
 *
 * @tparam T Type of the consumed products
 */
template <typename T>
class MultiConsumer : public IConsumer<T>
{
private:
  std::vector<std::shared_ptr<IConsumer<T>>> consumers_;

public:
  /*!
   * \brief Creates a new MultiConsumer object.
   *
   * \param consumers The list of consumers that should all consume given products
   */
  MultiConsumer(std::vector<std::shared_ptr<IConsumer<T>>> consumers) : consumers_(consumers)
  {
  }

  /*!
   * \brief Adds a new consumer to the list of consumers
   *
   * \param consumer Consumer that should be added to the list
   */
  void addConsumer(std::shared_ptr<IConsumer<T>> consumer)
  {
    std::lock_guard<std::mutex> lk(consumer_list);
    consumers_.push_back(consumer);
  }

  /*!
   * \brief Remove a consumer from the list of consumers
   *
   * \param consumer Consumer that should be removed from the list
   */
  void removeConsumer(std::shared_ptr<IConsumer<T>> consumer)
  {
    std::lock_guard<std::mutex> lk(consumer_list);
    auto it = std::find(consumers_.begin(), consumers_.end(), consumer);
    if (it == consumers_.end())
    {
      URCL_LOG_ERROR("Unable to remove consumer as it is not part of the consumer list");
      return;
    }
    consumers_.erase(it);
  }

  /*!
   * \brief Sets up all registered consumers.
   */
  virtual void setupConsumer()
  {
    for (auto& con : consumers_)
    {
      con->setupConsumer();
    }
  }
  /*!
   * \brief Tears down all registered consumers.
   */
  virtual void teardownConsumer()
  {
    for (auto& con : consumers_)
    {
      con->teardownConsumer();
    }
  }
  /*!
   * \brief Stops all registered consumers.
   */
  virtual void stopConsumer()
  {
    for (auto& con : consumers_)
    {
      con->stopConsumer();
    }
  }
  /*!
   * \brief Triggers timeout functionality for all registered consumers.
   */
  virtual void onTimeout()
  {
    for (auto& con : consumers_)
    {
      con->onTimeout();
    }
  }

  /*!
   * \brief Consumes a given product with all registered consumers.
   *
   * \param product Shared pointer to the product to be consumed.
   *
   * \returns Success of the consumption.
   */
  bool consume(std::shared_ptr<T> product)
  {
    std::lock_guard<std::mutex> lk(consumer_list);
    bool res = true;
    for (auto& con : consumers_)
    {
      if (!con->consume(product))
        res = false;
    }
    return res;
  }

private:
  std::mutex consumer_list;
};

/*!
 * \brief Parent class for arbitrary producers of packages.
 *
 * @tparam T Type of the produced products
 */
template <typename T>
class IProducer
{
public:
  /*!
   * \brief Set-up functionality of the producers.
   *
   * \param max_num_tries Maximum number of connection attempts before counting the connection as
   * failed. Unlimited number of attempts when set to 0.
   * \param reconnection_time time in between connection attempts to the server
   */
  virtual void setupProducer(const size_t max_num_tries = 0,
                             const std::chrono::milliseconds reconnection_time = std::chrono::seconds(10))
  {
  }
  /*!
   * \brief Fully tears down the producer - by default no difference to stopping it.
   */
  virtual void teardownProducer()
  {
    stopProducer();
  }
  /*!
   * \brief Stops the producer.
   */
  virtual void stopProducer()
  {
  }

  virtual void startProducer()
  {
  }

  /*!
   * \brief Reads packages from some source and produces corresponding objects.
   *
   * \param products Vector of unique pointers to be filled with produced packages.
   *
   * \returns Success of the package production.
   */
  virtual bool tryGet(std::vector<std::unique_ptr<T>>& products) = 0;
};

/*!
 * \brief Parent class for notifiers.
 */
class INotifier
{
public:
  /*!
   * \brief Start notification.
   */
  virtual void started(std::string name)
  {
  }
  /*!
   * \brief Stop notification.
   */
  virtual void stopped(std::string name)
  {
  }
};

/*!
 * \brief The Pipepline manages the production and optionally consumption of packages. Cyclically
 * the producer is called and returned packages are saved in a queue. This queue is then either also
 * cyclically utilized by the registered consumer or can be externally used.
 *
 * @tparam T Type of the managed packages
 */
template <typename T>
class Pipeline
{
public:
  typedef std::chrono::high_resolution_clock Clock;
  typedef Clock::time_point Time;
  /*!
   * \brief Creates a new Pipeline object, registering producer, consumer and notifier.
   * Additionally, an empty queue is initialized.
   *
   * \param producer The producer to run in the pipeline
   * \param consumer The consumer to run in the pipeline
   * \param name The pipeline's name
   * \param notifier The notifier to use
   * \param producer_fifo_scheduling Should the producer thread use FIFO scheduling?
   */
  Pipeline(IProducer<T>& producer, IConsumer<T>* consumer, std::string name, INotifier& notifier,
           const bool producer_fifo_scheduling = false)
    : producer_(producer)
    , consumer_(consumer)
    , name_(name)
    , notifier_(notifier)
    , queue_{ 32 }
    , running_{ false }
    , producer_fifo_scheduling_(producer_fifo_scheduling)
  {
  }
  /*!
   * \brief Creates a new Pipeline object, registering producer and notifier while no consumer is
   * used. Additionally, an empty queue is initialized.
   *
   * \param producer The producer to run in the pipeline
   * \param name The pipeline's name
   * \param notifier The notifier to use
   * \param producer_fifo_scheduling Should the producer thread use FIFO scheduling?
   */
  Pipeline(IProducer<T>& producer, std::string name, INotifier& notifier, const bool producer_fifo_scheduling = false)
    : producer_(producer)
    , consumer_(nullptr)
    , name_(name)
    , notifier_(notifier)
    , queue_{ 32 }
    , running_{ false }
    , producer_fifo_scheduling_(producer_fifo_scheduling)
  {
  }

  /*!
   * \brief The Pipeline object's destructor, stopping the pipeline and joining all running threads.
   */
  virtual ~Pipeline()
  {
    URCL_LOG_DEBUG("Destructing pipeline");
    stop();
  }

  /*!
   * \brief Initialize the pipeline. Internally calls setup of producer and consumer.
   *
   * \param max_num_tries Maximum number of connection attempts before counting the connection as
   * failed. Unlimited number of attempts when set to 0.
   * \param reconnection_time time in between connection attempts to the server
   */
  void init(const size_t max_num_tries = 0,
            const std::chrono::milliseconds reconnection_time = std::chrono::seconds(10))
  {
    producer_.setupProducer(max_num_tries, reconnection_time);
    if (consumer_ != nullptr)
      consumer_->setupConsumer();
  }

  /*!
   * \brief Starts the producer and, if existing, the consumer in new threads.
   */
  void run()
  {
    if (running_)
      return;

    running_ = true;
    producer_.startProducer();
    pThread_ = std::thread(&Pipeline::runProducer, this);
    if (consumer_ != nullptr)
      cThread_ = std::thread(&Pipeline::runConsumer, this);
    notifier_.started(name_);
  }

  /*!
   * \brief Stops the pipeline and all running threads.
   */
  void stop()
  {
    if (!running_)
      return;

    URCL_LOG_DEBUG("Stopping pipeline! <%s>", name_.c_str());

    running_ = false;

    producer_.stopProducer();
    if (pThread_.joinable())
    {
      pThread_.join();
    }
    if (cThread_.joinable())
    {
      cThread_.join();
    }
    notifier_.stopped(name_);
  }

  /*!
   * \brief Returns the most recent package in the queue. Can be used instead of registering a consumer. If the queue
   * already contains one or more items, the queue will be flushed and the newest item will be returned. If there is no
   * item inside the queue, the function will wait for \p timeout for a new package
   *
   * \param product Unique pointer to be set to the package
   * \param timeout Time to wait if no package is in the queue before returning
   *
   * \returns
   */
  bool getLatestProduct(std::unique_ptr<T>& product, std::chrono::milliseconds timeout)
  {
    // If the queue has more than one package, get the latest one.
    bool res = false;
    while (queue_.try_dequeue(product))
    {
      res = true;
    }

    // If the queue is empty, wait for a package.
    return res || queue_.wait_dequeue_timed(product, timeout);
  }

private:
  IProducer<T>& producer_;
  IConsumer<T>* consumer_;
  std::string name_;
  INotifier& notifier_;
  moodycamel::BlockingReaderWriterQueue<std::unique_ptr<T>> queue_;
  std::atomic<bool> running_;
  std::thread pThread_, cThread_;
  bool producer_fifo_scheduling_;

  void runProducer()
  {
    URCL_LOG_DEBUG("Starting up producer");
    if (producer_fifo_scheduling_)
    {
#ifndef WIN32
      pthread_t this_thread = pthread_self();
      const int max_thread_priority = sched_get_priority_max(SCHED_FIFO);
      setFiFoScheduling(this_thread, max_thread_priority);
#endif
    }
    std::vector<std::unique_ptr<T>> products;
    while (running_)
    {
      if (!producer_.tryGet(products))
      {
        producer_.teardownProducer();
        running_ = false;
        break;
      }

      for (auto& p : products)
      {
        if (!queue_.try_emplace(std::move(p)))
        {
          URCL_LOG_ERROR("Pipeline producer overflowed! <%s>", name_.c_str());
        }
      }

      products.clear();
    }
    URCL_LOG_DEBUG("Pipeline producer ended! <%s>", name_.c_str());
    notifier_.stopped(name_);
  }

  void runConsumer()
  {
    std::unique_ptr<T> product;
    while (running_)
    {
      // timeout was chosen because we should receive messages
      // at roughly 125hz (every 8ms) and have to update
      // the controllers (i.e. the consumer) with *at least* 125Hz
      // So we update the consumer more frequently via onTimeout
      if (!queue_.wait_dequeue_timed(product, std::chrono::milliseconds(8)))
      {
        consumer_->onTimeout();
        continue;
      }

      if (!consumer_->consume(std::move(product)))
      {
        consumer_->teardownConsumer();
        running_ = false;
        break;
      }
    }
    consumer_->stopConsumer();
    URCL_LOG_DEBUG("Pipeline consumer ended! <%s>", name_.c_str());
    notifier_.stopped(name_);
  }
};
}  // namespace comm
}  // namespace urcl
