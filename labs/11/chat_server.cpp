#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "chat_message.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant {
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
  virtual void disconnect() = 0;
  std::string nick;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room {
 public:
  void join(chat_participant_ptr participant) {
    bool nick_exists = false;
    for (auto p : participants_) {
      if (p->nick == participant->nick) {
        nick_exists = true;
      }
    }
    if (nick_exists) {
      std::cout << "Disconnecting: " << participant->nick << std::endl;
      participant->disconnect();
      return;
    }
    chat_message all_participants;
    std::cout << "Number of participants: " << participants_.size() << std::endl;
    int i = 0;
    for (auto p : participants_) {
      memcpy(all_participants.body() + i * chat_message::nick_length,
             p->nick.data(), chat_message::nick_length);
      ++i;
    }
    all_participants.body_length(participants_.size() * 16);
    all_participants.nick()[0] = 0;
    participants_.insert(participant);
    // std::cout << "Sending: " << all_participants.body() << std::endl;
    // std::cout << all_participants.body_length() << std::endl;
    // participant->deliver(all_participants);
    for (auto msg: recent_msgs_)
      participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant) {
    participants_.erase(participant);
  }

  void deliver(const chat_message& msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant: participants_)
      participant->deliver(msg);
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session> {
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room) {
  }

  void start() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(nick_msg_.data(), 20),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec) {
        std::cout << "Nick joined: " << nick_msg_.nick() << std::endl;
        nick = nick_msg_.nick();
        room_.join(shared_from_this());
        do_read_header();
      } else {
        std::cerr << "failed to receive nickname!" << std::endl;
        socket_.close();
      }
    });
  }

  void deliver(const chat_message& msg) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
      do_write();
    }
  }

  virtual void disconnect() override {
    auto self(shared_from_this());
    nick_msg_.body_length(0);
    boost::asio::async_write(socket_,
        boost::asio::buffer(nick_msg_.data(), nick_msg_.length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      socket_.close();
    });
  }

private:
  void do_read_header() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), 20),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec && read_msg_.decode_header()) {
        do_read_body();
      } else {
        room_.leave(shared_from_this());
      }
    });
  }

  void do_read_body() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec) {
        room_.deliver(read_msg_);
        do_read_header();
      } else {
        room_.leave(shared_from_this());
      }
    });
  }

  void do_write() {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t length) {
      if (!ec) {
        // std::cout << "Delivered: " << write_msgs_.front().length() << " " << length << std::endl;
        write_msgs_.pop_front();
        if (!write_msgs_.empty()) {
          do_write();
        }
      } else {
        room_.leave(shared_from_this());
      }
    });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message nick_msg_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class chat_server {
public:
  chat_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service) {
    do_accept();
  }

private:
  void do_accept() {
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
      if (!ec) {
        std::make_shared<chat_session>(std::move(socket_), room_)->start();
      }
      do_accept();
    });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: chat_server <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));
    chat_server server(io_service, endpoint);
    std::list<chat_server> servers;

    io_service.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
