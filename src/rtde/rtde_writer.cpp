// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK ----------------------------------------------
// Copyright 2019 FZI Forschungszentrum Informatik
// Created on behalf of Universal Robots A/S
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// -- END LICENSE BLOCK ------------------------------------------------

//----------------------------------------------------------------------
/*!\file
 *
 * \author  Tristan Schnell schnell@fzi.de
 * \date    2019-07-25
 *
 */
//----------------------------------------------------------------------

#ifdef WIN32
#include <asio.hpp>
#endif

#include "ur_client_library/rtde/rtde_writer.h"

namespace urcl
{
namespace rtde_interface
{
RTDEWriter::RTDEWriter(comm::URStream<RTDEPackage>* stream, const std::vector<std::string>& recipe)
  : stream_(stream), recipe_(recipe), queue_{ 32 }, running_(false), package_(recipe_)
{
}

void RTDEWriter::init(uint8_t recipe_id)
{
  recipe_id_ = recipe_id;
  package_.initEmpty();
  running_ = true;
  writer_thread_ = std::thread(&RTDEWriter::run, this);
}

void RTDEWriter::run()
{
  uint8_t buffer[4096];
  size_t size;
  size_t written;
  std::unique_ptr<DataPackage> package;
  while (running_)
  {
    if (queue_.wait_dequeue_timed(package, 1000000))
    {
      package->setRecipeID(recipe_id_);
      size = package->serializePackage(buffer);
      stream_->write(buffer, size, written);
    }
  }
  URCL_LOG_DEBUG("Write thread ended.");
}

bool RTDEWriter::sendSpeedSlider(double speed_slider_fraction)
{
  if (speed_slider_fraction > 1.0 || speed_slider_fraction < 0.0)
  {
    std::stringstream ss;
    ss << "Speed slider fraction should be between 0 and 1. The speed slider fraction is "
       << static_cast<int>(speed_slider_fraction);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }

  std::lock_guard<std::mutex> guard(package_mutex_);
  uint32_t mask = 1;
  bool success = true;
  success = package_.setData("speed_slider_mask", mask);
  success = success && package_.setData("speed_slider_fraction", speed_slider_fraction);

  if (success)
  {
    if (!queue_.try_emplace(std::unique_ptr<DataPackage>(new DataPackage(package_))))
    {
      return false;
    }
  }
  mask = 0;
  success = package_.setData("speed_slider_mask", mask);
  return success;
}

bool RTDEWriter::sendStandardDigitalOutput(uint8_t output_pin, bool value)
{
  if (output_pin > 7)
  {
    std::stringstream ss;
    ss << "Standard digital output pins goes from 0 to 7. The output pin to change is " << static_cast<int>(output_pin);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }

  std::lock_guard<std::mutex> guard(package_mutex_);
  uint8_t mask = pinToMask(output_pin);
  bool success = true;
  uint8_t digital_output;
  if (value)
  {
    digital_output = 255;
  }
  else
  {
    digital_output = 0;
  }
  success = package_.setData("standard_digital_output_mask", mask);
  success = success && package_.setData("standard_digital_output", digital_output);

  if (success)
  {
    if (!queue_.try_emplace(std::unique_ptr<DataPackage>(new DataPackage(package_))))
    {
      return false;
    }
  }
  mask = 0;
  success = package_.setData("standard_digital_output_mask", mask);
  return success;
}

bool RTDEWriter::sendConfigurableDigitalOutput(uint8_t output_pin, bool value)
{
  if (output_pin > 7)
  {
    std::stringstream ss;
    ss << "Configurable digital output pins goes from 0 to 7. The output pin to change is "
       << static_cast<int>(output_pin);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }

  std::lock_guard<std::mutex> guard(package_mutex_);
  uint8_t mask = pinToMask(output_pin);
  bool success = true;
  uint8_t digital_output;
  if (value)
  {
    digital_output = 255;
  }
  else
  {
    digital_output = 0;
  }
  success = package_.setData("configurable_digital_output_mask", mask);
  success = success && package_.setData("configurable_digital_output", digital_output);

  if (success)
  {
    if (!queue_.try_emplace(std::unique_ptr<DataPackage>(new DataPackage(package_))))
    {
      return false;
    }
  }
  mask = 0;
  success = package_.setData("configurable_digital_output_mask", mask);
  return success;
}

bool RTDEWriter::sendToolDigitalOutput(uint8_t output_pin, bool value)
{
  if (output_pin > 1)
  {
    std::stringstream ss;
    ss << "Tool digital output pins goes from 0 to 1. The output pin to change is " << static_cast<int>(output_pin);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }

  std::lock_guard<std::mutex> guard(package_mutex_);
  uint8_t mask = pinToMask(output_pin);
  bool success = true;
  uint8_t digital_output;
  if (value)
  {
    digital_output = 255;
  }
  else
  {
    digital_output = 0;
  }
  success = package_.setData("tool_digital_output_mask", mask);
  success = success && package_.setData("tool_digital_output", digital_output);

  if (success)
  {
    if (!queue_.try_emplace(std::unique_ptr<DataPackage>(new DataPackage(package_))))
    {
      return false;
    }
  }
  mask = 0;
  success = package_.setData("tool_digital_output_mask", mask);
  return success;
}

bool RTDEWriter::sendStandardAnalogOutput(uint8_t output_pin, double value)
{
  if (output_pin > 1)
  {
    std::stringstream ss;
    ss << "Standard analog output goes from 0 to 1. The output pin to change is " << static_cast<int>(output_pin);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }
  if (value > 1.0 || value < 0.0)
  {
    std::stringstream ss;
    ss << "Analog output value should be between 0 and 1. The value is " << static_cast<int>(value);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }

  std::lock_guard<std::mutex> guard(package_mutex_);
  uint8_t mask = pinToMask(output_pin);
  // default to current for now, as no functionality to choose included in set io service
  uint8_t output_type = 0;
  bool success = true;
  success = package_.setData("standard_analog_output_mask", mask);
  success = success && package_.setData("standard_analog_output_type", output_type);
  success = success && package_.setData("standard_analog_output_0", value);
  success = success && package_.setData("standard_analog_output_1", value);

  if (success)
  {
    if (!queue_.try_emplace(std::unique_ptr<DataPackage>(new DataPackage(package_))))
    {
      return false;
    }
  }
  mask = 0;
  success = package_.setData("standard_analog_output_mask", mask);
  return success;
}

uint8_t RTDEWriter::pinToMask(uint8_t pin)
{
  if (pin > 7)
  {
    return 0;
  }

  return 1 << pin;
}

bool RTDEWriter::sendInputBitRegister(uint32_t register_id, bool value)
{
  if (register_id < 64 || register_id > 127)
  {
    std::stringstream ss;
    ss << "Input bit register goes from 64 to 127. The register id to change is " << static_cast<int>(register_id);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }

  std::lock_guard<std::mutex> guard(package_mutex_);
  std::stringstream ss;
  ss << "input_bit_register_" << register_id;

  bool success = package_.setData(ss.str(), value);

  if (success)
  {
    if (!queue_.try_emplace(std::unique_ptr<DataPackage>(new DataPackage(package_))))
    {
      return false;
    }
  }
  return success;
}

bool RTDEWriter::sendInputIntRegister(uint32_t register_id, int32_t value)
{
  if (register_id < 24 || register_id > 47)
  {
    std::stringstream ss;
    ss << "Input int register goes from 24 to 47. The register id to change is " << static_cast<int>(register_id);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }

  std::lock_guard<std::mutex> guard(package_mutex_);
  std::stringstream ss;
  ss << "input_int_register_" << register_id;

  bool success = package_.setData(ss.str(), value);

  if (success)
  {
    if (!queue_.try_emplace(std::unique_ptr<DataPackage>(new DataPackage(package_))))
    {
      return false;
    }
  }
  return success;
}

bool RTDEWriter::sendInputDoubleRegister(uint32_t register_id, double value)
{
  if (register_id < 24 || register_id > 47)
  {
    std::stringstream ss;
    ss << "Input double register goes from 24 to 47. The register id to change is " << static_cast<int>(register_id);
    URCL_LOG_ERROR(ss.str().c_str());
    return false;
  }

  std::lock_guard<std::mutex> guard(package_mutex_);
  std::stringstream ss;
  ss << "input_double_register_" << register_id;

  bool success = package_.setData(ss.str(), value);

  if (success)
  {
    if (!queue_.try_emplace(std::unique_ptr<DataPackage>(new DataPackage(package_))))
    {
      return false;
    }
  }
  return success;
}

}  // namespace rtde_interface
}  // namespace urcl
