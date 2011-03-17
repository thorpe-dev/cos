#include "threads/synch.h"

struct frame {
  void* page_location;
  struct thread* owner; //TODO: Is this necessary?
  // TODO: Add extra information for eviction
};

void frame_init(int size);
void frame_add(int frame_index, void* page_location, int count);
void frame_del(int frame_index, int count);

