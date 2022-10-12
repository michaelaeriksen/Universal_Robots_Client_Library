#include <map>

#include "ur_client_library/comm/Server.h"
#include "ur_client_library/comm/SocketListener.h"
#include "ur_client_library/comm/SocketConnection.h"

namespace urcl::comm
{
class ServerMessageProcessor : public MessageProcessor
{
public:
  ServerMessageProcessor(ServerImpl* pServerImpl) : m_pServerImpl(pServerImpl)
  {
  }
  ~ServerMessageProcessor() override = default;

public:
  void ProcessMessage(const std::unique_ptr<Message>& message, std::unique_ptr<Transport>&& transport,
                      SocketConnection* connection_base) override;

private:
  ServerImpl* m_pServerImpl;
};

class ServerImpl
{
public:
  ServerImpl(const int port) : port_(port)
  {
    m_pServer = std::make_shared<comm::SocketServer>(std::make_shared<ServerMessageProcessor>(this));

    auto listener = std::make_shared<TcpListener>(m_io_service, asio::ip::address(), port);
    m_pServer->attach_listener(std::static_pointer_cast<SocketListener>(listener));

    m_pServer->SetConnectCallback([this](const std::shared_ptr<SocketConnection> pConnection) {
      client_count++;
      if (max_clients_allowed_ != 0 && client_count > max_clients_allowed_)
      {
        assert(false);
      }

      size_t index = 0;
      {
        std::lock_guard<std::mutex> lock(m_Mutex);
        index = m_pConnections.size();
        m_pConnections[pConnection] = index;
      }

      new_connection_callback_(index);
    });

    m_pServer->SetDisconnectCallback([this](const std::shared_ptr<SocketConnection> pConnection) {
      client_count--;

      {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto iter = m_pConnections.find(pConnection);
        if (iter != m_pConnections.end())
        {
          disconnect_callback_(iter->second);

          m_pConnections.erase(iter);
        }
        else
        {
          assert(false);
        }
      }
    });
  }

  ~ServerImpl()
  {
    Shutdown();
  }

public:
  void Start()
  {
    m_work = std::make_unique<asio::io_service::work>(m_io_service);

    if (m_pServer)
    {
      m_pServer->start();
    }

    m_WorkerThread = std::thread([this]() { m_io_service.run(); });
  }

  void Shutdown()
  {
    if (m_work)
    {
      m_io_service.dispatch(
        [this]() {
        if (m_work.get())
        {
          if (m_pServer)
          {
            m_pServer->shutdown();
          }

          m_work.reset();
          m_io_service.poll();
          m_io_service.stop();
        }
      });
    }

    if (m_WorkerThread.joinable())
    {
      m_WorkerThread.join();
    }
  }
  
  void setConnectCallback(std::function<void(const int)> func)
  {
    new_connection_callback_ = func;
  }

  void setDisconnectCallback(std::function<void(const int)> func)
  {
    disconnect_callback_ = func;
  }

  void setMessageCallback(std::function<void(const int, char*, int)> func)
  {
    message_callback_ = func;
  }

  void setMaxClientsAllowed(const uint32_t& max_clients_allowed)
  {
    max_clients_allowed_ = max_clients_allowed;
  }

public:
  int port_{};
  uint32_t max_clients_allowed_{};
  uint32_t client_count{};
  std::function<void(const int)> new_connection_callback_;
  std::function<void(const int)> disconnect_callback_;
  std::function<void(const int, char* buffer, int nbytesrecv)> message_callback_;

  asio::io_service m_io_service;
  std::shared_ptr<asio::io_service::work> m_work;

  std::shared_ptr<SocketServer> m_pServer;

  std::thread m_WorkerThread;

  std::mutex m_Mutex;
  std::map<std::shared_ptr<SocketConnection>, int32_t> m_pConnections;
};

Server::Server(const int port) : m_pImpl(std::make_shared<ServerImpl>(port))
{
}

Server::~Server()
{
}

void Server::start()
{
  m_pImpl->Start();
}

void Server::shutdown()
{
  m_pImpl->Shutdown();
}

bool Server::write(const int fd, const uint8_t* buf, const size_t buf_len, size_t& written)
{
  std::shared_ptr<SocketConnection> pConnection;

  {
    std::lock_guard<std::mutex> lock(m_pImpl->m_Mutex);
    for (auto iter : m_pImpl->m_pConnections)
    {
      if (iter.second == fd)
      {
        pConnection = iter.first;
      }
    }
  }

  asio::error_code error;
  if (pConnection)
  {
    pConnection->write(buf, buf_len, error);
    return true;
  }
  else
  {
    assert(false);
    return false;
  }
}

void Server::setConnectCallback(std::function<void(const int)> func)
{
  m_pImpl->setConnectCallback(func);
}

void Server::setDisconnectCallback(std::function<void(const int)> func)
{
  m_pImpl->setDisconnectCallback(func);
}

void Server::setMessageCallback(std::function<void(const int, char*, int)> func)
{
  m_pImpl->setMessageCallback(func);
}

void Server::setMaxClientsAllowed(const uint32_t& max_clients_allowed)
{
  m_pImpl->max_clients_allowed_ = max_clients_allowed;
}

void ServerMessageProcessor::ProcessMessage(const std::unique_ptr<Message>& message,
                                            std::unique_ptr<Transport>&& transport, SocketConnection* connection_base)
{
  int32_t index = -1;
  {
    std::lock_guard<std::mutex> lock(m_pServerImpl->m_Mutex);
    for (auto pConnection : m_pServerImpl->m_pConnections)
    {
      if (pConnection.first.get() == connection_base)
      {
        index = pConnection.second;
      }
    }
  }

  if (index < 0)
  {
    assert(false);
  }

  const auto& bytes = message->GetBytes();
  m_pServerImpl->message_callback_(index, (char*)bytes.data(), bytes.size());
}

}  // namespace urcl::comm