#include "mem.h"
#include <iostream>

struct simple_test
{
    int f1;
    int f2;
};

struct test_struct
{
    long double another_field;
    int a_field;
    bool a_third_field;
};

class test_klass
{
  public:
    test_klass(double d1, double d2)
      : some_data(d1)
      , more_data(d2)
    {
    }

    double get_sum() { return some_data + more_data; }

  private:
    double some_data;
    double more_data;
};

int
main()
{

    byte_ptr* memory = new byte_ptr[1024];
    std::cout << sizeof(long double) << std::endl;
    std::cout << (mem_ptr*)memory << std::endl;

    mem::ByteArena arena(memory, 1024);

    std::cout << "arena bytes: " << arena.arena_bytes
              << " idx: " << arena.next_free_idx
              << " mem at: " << (mem_ptr*)arena.mem_begin << std::endl;
    std::cout << std::endl;

    byte_ptr* bytes = arena.alloc_bytes(21);

    std::cout << "alloc'd 21 bytes" << std::endl;
    std::cout << "arena bytes: " << arena.arena_bytes
              << " idx: " << arena.next_free_idx
              << " mem at: " << (mem_ptr*)arena.mem_begin
              << " bytes at: " << (mem_ptr*)bytes
              << std::endl;
    std::cout << std::endl;

    //simple_test* st = arena.alloc_simple<simple_test>();
    //std::cout << "alloc'd a simple test obj" << std::endl;
    //std::cout << "arena bytes: " << arena.arena_bytes
              //<< " idx: " << arena.next_free_idx
              //<< " mem at: " << (mem_ptr*)arena.mem_begin
              //<< " obj at: " << (mem_ptr*)st
              //<< std::endl;
//
    //st->f1 = 35;
    //st->f2 = 34;
    //std::cout << st->f1 << "+" << st->f2 << "=" << st->f1 + st->f2 << std::endl;
    //std::cout << std::endl;

    test_struct* ts = arena.alloc_simple<test_struct>();
    std::cout << "alloc'd a more complex obj of size/align "
              << sizeof(test_struct) << "/" << alignof(test_struct) << std::endl;
    std::cout << "arena bytes: " << arena.arena_bytes
              << " idx: " << arena.next_free_idx
              << " mem at: " << (mem_ptr*)arena.mem_begin
              << " obj at: " << (mem_ptr*)ts
              << std::endl;
    std::cout << std::endl;

    test_klass* tk = arena.alloc_obj<test_klass>(0.69, 4.20);
    std::cout << "alloc'd a more complex obj" << std::endl;
    std::cout << "arena bytes: " << arena.arena_bytes
              << " idx: " << arena.next_free_idx
              << " mem at: " << (mem_ptr*)arena.mem_begin
              << " obj at: " << (mem_ptr*)tk
              << std::endl;
    std::cout << tk->get_sum() << std::endl;

    return 0;
}
