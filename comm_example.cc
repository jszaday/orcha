#include <iostream>
#include "comm.hh"
#include "core.hh"

int main(int argc, char **argv) {
  orcha::comm::initialize(argc, argv);

  auto rank = orcha::comm::rank(orcha::comm::k_global),
       size = orcha::comm::size(orcha::comm::k_global);

  std::cout << "[" << rank << "] finished initializing." << std::endl;

  if (rank % 2 == 0) {
    auto neighbor = (rank + 1) % size;
    std::cout << "[" << rank << "] requesting value from " << neighbor << "." << std::endl;
    auto val = orcha::comm::request_value(orcha::comm::k_global, neighbor, rank);
    val.wait();
    std::cout << "[" << rank << "] received value: " << val.get() << std::endl;
  } else {
    orcha::k_pending_[orcha::comm::make_tag(rank, rank - 1)] = std::async(std::launch::async, [rank] {
      return std::string("Hello from ") + std::to_string(rank) + std::string("!");
    });
  }

  orcha::comm::barrier(orcha::comm::k_global);

  orcha::comm::finalize();

  return 0;
}
