
#include <iostream>
#include "looping_thread.hpp"

int main() {
	{
		LoopingThread loop(std::chrono::seconds(2), [] {
			std::cout << "(worker) Waiting for 1s" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "(worker) Waited for 1s" << std::endl;
		});
		std::cout << "(main) Waiting for 5s" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(7200));
		std::cout << "(main) Waited for 5s" << std::endl;
	}
	std::cout << "(main) Destroyed successfully" << std::endl;
	return 0;
}
