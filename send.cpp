#include "exr.h"
#include "message.h"
#include "unistd.h"
#include "zmqpp.h"
#include <iostream>

#include "slice.h"

void send_order(zmq::socket& sck_data, std::vector<Bucket> bucket_list,
                RGBA* pixels, int width, int height) {
   // maximum scratch space is the global bucket size
   std::unique_ptr<RGBA[]> bucket_pixels(new RGBA[BUCKET_SIZE * BUCKET_SIZE]);

   // loop over the bucket pixels and copy them into scratch space to form a
   // slice to pass to the message
   for (auto& b : bucket_list) {
      int bw = b.bounds.size().x;
      int bh = b.bounds.size().y;
      int spi = 0;
      for (int by = b.bounds.min.y; by < b.bounds.max.y; ++by) {
         for (int bx = b.bounds.min.x; bx < b.bounds.max.x; ++bx) {
            // copy pixel from the original image to the bucket scratch
            int bpi = by * width + bx;
            bucket_pixels[spi++] = pixels[bpi];
         }
      }

      Slice<RGBA> bucket_slice(bucket_pixels.get(), spi);

      zmq::message msg_fill;
      // first send the bucket back
      msg_fill.add(b);
      // then send the slice
      msg_fill.add(bucket_slice);
      sck_data.send(msg_fill);
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

   zmq::context context;

   // socket for sending bucket data
   zmq::socket sck_data(context, zmq::socket_type::push);
   sck_data.connect("tcp://135.19.24.125:35556");

   // socket for receiving orders
   zmq::socket sck_orders(context, zmq::socket_type::pull);
   sck_orders.connect("tcp://135.19.24.125:35555");

   while (true) {
      zmq::message msg_order;
      sck_orders.recv(msg_order);
      std::vector<Bucket> bucket_list;
      OrderId id = msg_order.get<OrderId>(0);
      switch (id) {
         case OrderId::Invalid:
            std::cerr << "Invalid order id" << std::endl;
            return -2;
         case OrderId::Header:
            std::cout << "Header" << std::endl;
            break;
         case OrderId::BucketList:
            bucket_list = msg_order.get_vector<Bucket>(1);
            std::cout << "Order of " << bucket_list.size() << " buckets"
                      << std::endl;
            send_order(sck_data, bucket_list, pixels.get(), width, height);
            break;
         case OrderId::End:
            std::cout << "End signal." << std::endl;
            return 0;
      }
   }
}
