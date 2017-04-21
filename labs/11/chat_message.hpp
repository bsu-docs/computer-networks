#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>

class chat_message
{
public:
  enum { header_length = 4 };
  enum { nick_length = 16 };
  enum { max_body_length = 512 };

  chat_message()
    : body_length_(0) {
    memset(nick(), 0, nick_length);
  }

  chat_message(const chat_message& m) {
    body_length_ = m.body_length_;
    for (int i = 0; i < m.length(); ++i) {
      data_[i] = m.data_[i];
    }
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  const char* nick() const
  {
    return data_ + header_length;
  }

  char* nick()
  {
    return data_ + header_length;
  }

  std::size_t length() const
  {
    return header_length + nick_length + body_length_;
  }

  const char* body() const
  {
    return data_ + header_length + nick_length;
  }

  char* body()
  {
    return data_ + header_length + nick_length;
  }

  std::size_t body_length() const
  {
    return body_length_;
  }

  void body_length(std::size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > max_body_length)
      body_length_ = max_body_length;
  }

  bool decode_header()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    body_length_ = std::atoi(header);
    if (body_length_ > max_body_length)
    {
      body_length_ = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    char header[header_length + 1] = "";
    std::sprintf(header, "%4d", body_length_);
    std::memcpy(data_, header, header_length);
  }

private:
  char data_[header_length + nick_length + max_body_length];
  std::size_t body_length_;
};

#endif // CHAT_MESSAGE_HPP
