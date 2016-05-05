#include "zmqpp.h"
#include "exr.h"
#include "message.h"
#include <thread>
#include <iostream>
#include <vector>

void accumulate_bucket(RGBA* pixels, int width, int height,
                       const Bucket& b,
                       const std::vector<RGBA>& bucket_pixels) {
   
      int bw = b.bounds.size().x;
      int bh = b.bounds.size().y;
      int spi = 0;
      for (int by = b.bounds.min.y; by < b.bounds.max.y; ++by) {
         for (int bx = b.bounds.min.x; bx < b.bounds.max.x; ++bx) {
            int bpi = by * width + bx;
            pixels[bpi] = bucket_pixels[spi++];
         }
      }
}

int main(int argc, char** argv) {
   if (argc < 2) {
      std::cerr << "send <file.exr>\n" << std::endl;
      return -1;
   }

   // read exr to send
   int width = 0;
   int height = 0;
   std::unique_ptr<RGBA[]> pixels = read_rgba_exr(argv[1], width, height);
   // set pixels to black to accumulate later
   memset(pixels.get(), 0, sizeof(RGBA) * width * height);
   std::cout << "Loaded " << argv[1] << " " << width << "x" << height
             << std::endl;

   // make bucket list
   std::vector<Bucket> bucket_list;
   for (int y = 0; y < height; y += BUCKET_SIZE) {
      for (int x = 0; x < width; x += BUCKET_SIZE) {
         Bucket b;
         b.id = (int)bucket_list.size();
         b.bounds.min.x = x;
         b.bounds.min.y = y;
         b.bounds.max.x = std::min(x + BUCKET_SIZE, width);
         b.bounds.max.y = std::min(y + BUCKET_SIZE, height);
         bucket_list.emplace_back(b);
      }
   }

   size_t num_buckets = bucket_list.size();

   zmq::context context;

   // socket for sending order requests
   zmq::socket sck_orders(context, zmq::socket_type::push);
   sck_orders.bind("tcp://*:35555");

   std::thread thrd_recv([&context, &num_buckets, &pixels, width, height]() {
      // socket for receiving bucket data
      zmq::socket sck_data(context, zmq::socket_type::pull);
      sck_data.bind("tcp://*:35556");

      int num_filled = 0;
      while (true) {
         zmq::message msg_fill;
         sck_data.recv(msg_fill);
         Bucket b = msg_fill.get<Bucket>();
         std::vector<RGBA> bucket_pixels = msg_fill.get_vector<RGBA>(1);
         accumulate_bucket(pixels.get(), width, height, b, bucket_pixels);
         std::cout << "Filled order " << b.id << std::endl;
         num_filled++;
         if (num_filled == num_buckets) {
            write_rgba_exr("out.exr", width, height, pixels.get());
            break;
         }
      }
   });

   zmq::message msg_order;
   msg_order.add(OrderId::BucketList);
   msg_order.add(bucket_list);
   sck_orders.send(msg_order);

   thrd_recv.join();

   zmq::message msg_end;
   msg_end.add(OrderId::End);
   sck_orders.send(msg_end);
   std::cout << "done." << std::endl;

   return 0;
}
