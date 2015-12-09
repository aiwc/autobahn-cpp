///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Tavendo GmbH
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#include <map>
#include <memory>
#include <iostream>

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread/future.hpp>
#include <boost/asio.hpp>

using namespace std;
using namespace boost;


struct rpcsvc {

   rpcsvc (asio::io_service& io) : m_io(io), m_call_id(0) {
   }

   struct Call {
      std::shared_ptr<asio::deadline_timer> m_timer;
      promise<float> m_promise;
   };

   future<float> slowsquare(float x, float delay) {

      m_call_id += 1;
      m_calls[m_call_id] = Call();

      std::shared_ptr<asio::deadline_timer>
         timer(new asio::deadline_timer(m_io, posix_time::seconds(delay)));

      m_calls[m_call_id].m_timer = timer;

      int call_id = m_call_id;

      timer->async_wait(
         [=](system::error_code ec) {
            if (!ec) {

               // Question 1:
               //
               // m_call_id seems to be captured by value, though we
               // said [=] in the lambda .. why? We workaround by
               // using a local var in the enclosing method body.
               //
               cout << m_call_id << " - " << call_id << endl;

               m_calls[call_id].m_promise.set_value(x * x);
               m_calls.erase(call_id);
            } else {
               cout << "Error in timer: " << ec << endl;
            }
         }
      );

      cout << "call " << m_call_id << " issued" << endl;

      return m_calls[m_call_id].m_promise.get_future();
   }

   asio::io_service& m_io;
   int m_call_id;
   map<int, Call> m_calls;
};


int main () {

   try {
      asio::io_service io;

      rpcsvc rpc(io);

      auto f1 = rpc.slowsquare(2, 2).then([](future<float> f) {
         cout << "call 1 returned" << endl;
         cout << "result 1: " << f.get() << endl;
      });
/*
      auto f2 = rpc.slowsquare(3, 1.1).then([&rpc](future<float> f) {
         cout << "call 2 returned" << endl;
         cout << "result 2: " << f.get() << endl;

         auto f2b = rpc.slowsquare(5, 1.1).then([&rpc](future<float> f) {
            cout << "call 2b returned" << endl;
            cout << "result 2b: " << f.get() << endl;
            auto f2c = rpc.slowsquare(6, 0.6).then([](future<float> f) {
               cout << "call 2c returned" << endl;
               cout << "result 2c: " << f.get() << endl;
            });
            return f2c;
         });

         auto f2d = rpc.slowsquare(7, 0.2).then([](future<float> f) {
            cout << "call 1 returned" << endl;
            cout << "result 1: " << f.get() << endl;
         });

         return when_all(std::move(f2b), std::move(f2d));
      });
*/
      auto f3 = rpc.slowsquare(4, 1).then([](future<float> f) {
         cout << "call 3 returned" << endl;
         cout << "result 3: " << f.get() << endl;
      });
/*
      auto f12 = when_all(std::move(f1), std::move(f2));
      auto f12d = f12.then([](decltype(f12)) {
         cout << "call 1/2 done" << endl;
      });
*/
/*
      auto f23 = when_all(std::move(f2), std::move(f3));
      auto f23d = f23.then([](decltype(f23)) {
         cout << "call 2/3 done" << endl;
      });

      auto f123 = when_all(std::move(f12d), std::move(f23d));
      auto f123d = f123.then([](decltype(f123)) {
         cout << "all calls done" << endl;
      });
*/
      io.run();

      // Question 2:
      //
      // Neither f12 nor f12d (and etc) "induce" any work on
      // ASIO reactor .. so ASIO will end it's loop though the
      // continuation hasn't been executed yet. we workaround
      // by "manually" waiting .. but that seems suboptimal ..
      //
      //f12d.get();


      // Question 3:
      //
      // When above f23,f23d,f123,f123d and the line below
      // is commented in, the program will deadlock somehow.
      // Why? And is there a safe ("fool proof") way to avoid
      // this kind of issues?
      //f123d.get();
   }
   catch (std::exception& e) {
      cerr << e.what() << endl;
      return 1;
   }
   return 0;
}
