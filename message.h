#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <OpenEXR/ImathBox.h>

static const int BUCKET_SIZE = 256;

enum class OrderId : uint8_t {
   Invalid = 0,
   Header,
   BucketList,
   End
};

struct Bucket {
   int id = -1;
   Imath::Box2i bounds;
};

#endif
