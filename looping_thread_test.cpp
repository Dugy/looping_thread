
#include <iostream>
#include "looping_thread.hpp"

int main() {
	{
		LoopingThread loop(std::chrono::seconds(2), [] {
			std::cout << "(worker) Waiting for 1s" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "(worker) Waited for 1s" << std::endl;
		});
		std::cout << "(main) Waiting for 3.6s" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(3600));
		std::cout << "(main) Pausing for 0.3s" << std::endl;
		loop.pause(false);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		loop.resume();
		std::cout << "(main) Unpaused" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(4300));
		std::cout << "(main) Waited for 4.3s" << std::endl;
	}
	std::cout << "(main) Destroyed successfully" << std::endl;
	return 0;
}
