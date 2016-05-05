#ifndef ZMQPP_H
#define ZMQPP_H

#include <zmq.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>

#include "slice.h"

namespace zmq {

class exception : public std::exception {
   std::string _message;

  public:
   exception() : _message(zmq_strerror(zmq_errno())) {}

   virtual const char* what() const noexcept { return _message.c_str(); }
};

class context {
   void* _context;
   friend class socket;

  public:
   context() noexcept { _context = zmq_ctx_new(); }

   ~context() noexcept {
      zmq_ctx_destroy(_context);
      _context = nullptr;
   }

   // no copying
   context(const context&) = delete;
   context& operator=(const context&) = delete;
};

/// The part class wraps an individual zmq_msg_t object. Clients should use the
/// message type instead as that abstracts over the multiple parts
class part {
   zmq_msg_t _msg;
   bool _init = false;
   friend class message;
   friend class socket;

   void* data() noexcept { return zmq_msg_data(&_msg); }

  public:
   part() noexcept {
      zmq_msg_init(&_msg);
      _init = true;
   }

   ~part() noexcept {
      if (_init) {
         zmq_msg_close(&_msg);
         _init = false;
      }
   }

   // no copy
   part(const part&) = delete;
   part& operator=(const part&) = delete;

   part(part&& r) {
      memcpy(&_msg, &r._msg, sizeof(zmq_msg_t));
      _init = r._init;
      r._init = false;
   }

   template <typename T>
   part(const T& value) {
      zmq_msg_init_size(&_msg, sizeof(T));
      _init = true;
      memcpy(zmq_msg_data(&_msg), &value, sizeof(T));
   }

   template <typename T>
   part(const std::vector<T>& v) {
      size_t sz = v.size();
      // we'll store the size of the vector first, then the elements
      zmq_msg_init_size(&_msg, sizeof(T) * sz + sizeof(size_t));
      _init = true;
      size_t* szp = (size_t*)data();
      memcpy(szp, &sz, sizeof(size_t));
      // offset the data pointer past the size element
      szp += 1;
      // copy the vector elements
      memcpy(szp, v.data(), sizeof(T) * sz);
   }

   template <typename T>
   part(const Slice<T>& v) {
      size_t sz = v.size();
      // we'll store the size of the vector first, then the elements
      zmq_msg_init_size(&_msg, sizeof(T) * sz + sizeof(size_t));
      _init = true;
      size_t* szp = (size_t*)data();
      memcpy(szp, &sz, sizeof(size_t));
      // offset the data pointer past the size element
      szp += 1;
      // copy the vector elements
      memcpy(szp, v.data(), sizeof(T) * sz);
   }

   template <typename T>
   T get() noexcept {
      return *(T*)data();
   }

   template <typename T>
   std::vector<T> get_vector() noexcept {
      std::vector<T> v;
      // First sizeof(size_t) bytes is the size of the vector
      size_t* szp = (size_t*)data();
      size_t sz = *szp;
      v.resize(sz);
      // offset pointer past the size element
      szp += 1;
      // copy the vector elements
      // cast is not necessary here but it makes the intent clearer
      T* t = reinterpret_cast<T*>(szp);
      memcpy(&(v[0]), t, sizeof(T) * sz);
      return v;
   }
};

/// The message struct wraps a (possibly) multi-part message
class message {
   std::vector<part> _parts;
   friend class socket;

  public:
   message() {}
   // no copy
   message(const message&) = delete;
   message& operator=(const message&) = delete;

   template <typename T>
   void add(const T& value) noexcept {
      _parts.emplace_back(part(value));
   }

   template <typename T>
   T get(size_t part = 0) noexcept {
      assert(part < _parts.size());
      return _parts[part].get<T>();
   }

   template <typename T>
   std::vector<T> get_vector(size_t part = 0) noexcept {
      assert(part < _parts.size());
      return _parts[part].get_vector<T>();
   }

   size_t num_parts() const noexcept { return _parts.size(); }
};

enum class socket_type : int {
   req = ZMQ_REQ,
   rep = ZMQ_REP,
   dealer = ZMQ_DEALER,
   router = ZMQ_ROUTER,
   pub = ZMQ_PUB,
   sub = ZMQ_SUB,
   xpub = ZMQ_XPUB,
   xsub = ZMQ_XSUB,
   push = ZMQ_PUSH,
   pull = ZMQ_PULL,
   pair = ZMQ_PAIR,
   stream = ZMQ_STREAM
};

class socket {
   void* _socket;

  public:
   socket(context& ctx, socket_type t) {
      _socket = zmq_socket(ctx._context, (int)t);
      if (!_socket) {
         throw exception();
      }
   }

   ~socket() noexcept {
      zmq_close(_socket);
      _socket = nullptr;
   }

   // no copy
   socket(const socket&) = delete;
   socket& operator=(const socket&) = delete;

   /// Bind the socket to the given endpoint in order to receive messages
   void bind(const char* endpoint) {
      int r = zmq_bind(_socket, endpoint);
      if (r != 0) {
         throw exception();
      }
   }

   /// Connect the socket to the given endpoint in order to send messages
   void connect(const char* endpoint) {
      int r = zmq_connect(_socket, endpoint);
      if (r != 0) {
         throw exception();
      }
   }

   /// Send the given message
   void send(message& msg, int flags = 0) {
      for (size_t i = 0; i < msg._parts.size() - 1; ++i) {
         int r =
             zmq_msg_send(&msg._parts[i]._msg, _socket, flags | ZMQ_SNDMORE);
         if (r == -1) throw exception();
      }
      int r =
          zmq_msg_send(&msg._parts[msg._parts.size() - 1]._msg, _socket, flags);
      if (r == -1) throw exception();
   }

   /// Receive a message
   void recv(message& msg, int flags = 0) {
      int more = 1;
      do {
         part p;
         int r = zmq_msg_recv(&p._msg, _socket, flags);
         if (r == -1) {
            throw exception();
         }
         more = zmq_msg_more(&p._msg);
         msg._parts.emplace_back(std::move(p));
      } while (more);
   }
};
}

#endif
