#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include "chat_message.hpp"

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_client {
public:
  chat_client(boost::asio::io_service& io_service,
              tcp::resolver::iterator endpoint_iterator,
              std::string nick)
    : io_service_(io_service),
      socket_(io_service),
      nick_(nick) {
    memcpy(nick_msg_.nick(), nick.data(), nick.length());
    do_connect(endpoint_iterator);
  }

  void write(const chat_message& msg) {
    io_service_.post([this, msg]() {
      bool write_in_progress = !write_msgs_.empty();
      write_msgs_.push_back(msg);
      if (!write_in_progress) {
        do_write();
      }
    });
  }

  void close() {
    io_service_.post([this]() { socket_.close(); });
  }

private:
  void do_connect(tcp::resolver::iterator endpoint_iterator) {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator) {
      if (!ec) {
        do_send_nick();
      }
    });
  }

  void do_send_nick() {
    boost::asio::async_write(socket_,
        boost::asio::buffer(nick_msg_.data(), nick_msg_.length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec) {
        do_read_header();
      }
    });
  }

  void do_read_header() {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), 20),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec && read_msg_.decode_header()) {
        do_read_body();
      } else {
        socket_.close();
      }
    });
  }

  void do_read_body() {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec) {
        if (read_msg_.nick()[0] == 0) {
          std::cout << "\rList of participants: " << std::endl;
          std::cout << read_msg_.body_length() << std::endl;
          for (int i = 0; i < read_msg_.body_length() / 16; ++i) {
            std::cout << read_msg_.body() + i * chat_message::nick_length << std::endl;
          }
          do_read_header();
        } else if (read_msg_.body_length() == 0) {
          std::cerr << "\rNick \"" << nick_ << "\" is already in use" << std::endl;
          socket_.close();
          exit(1);
        } else {
          std::string sender_nick(read_msg_.nick());
          if (sender_nick != nick_) {
            std::cout << "\r";
            std::cout << read_msg_.nick() << "> ";
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "                          \n";
            std::cout << nick_ << "> " << std::flush;
          }
          do_read_header();
        }
      } else {
        socket_.close();
      }
    });
  }

  void do_write() {
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec) {
        write_msgs_.pop_front();
        if (!write_msgs_.empty()) {
          do_write();
        }
      } else {
        socket_.close();
      }
    });
  }

 private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  chat_message nick_msg_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  std::string nick_;
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 4) {
      std::cerr << "Usage: chat_client <nick> <host> <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    auto endpoint_iterator = resolver.resolve({ argv[2], argv[3] });
    chat_client c(io_service, endpoint_iterator, argv[1]);

    std::thread t([&io_service](){ io_service.run(); });

    char line[chat_message::max_body_length + 1];
    std::string nick(argv[1]);
    std::cout << nick << "> " << std::flush;
    while (std::cin.getline(line, chat_message::max_body_length + 1)) {
      if (strlen(line) > 0) {
        chat_message msg;
        msg.body_length(std::strlen(line));
        std::memcpy(msg.nick(), nick.data(), chat_message::nick_length);
        std::memcpy(msg.body(), line, msg.body_length());
        msg.encode_header();
        c.write(msg);
      }
      std::cout << nick << "> " << std::flush;
    }

    c.close();
    t.join();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
