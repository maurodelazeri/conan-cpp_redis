#include <string>
#include <cpp_redis/cpp_redis>
#include <cpp_redis/misc/macro.hpp>

#define ENABLE_SESSION = 1

#ifdef _WIN32
#include <Winsock2.h>
#endif /* _WIN32 */

int
main(void) {
#ifdef _WIN32
	//! Windows netword DLL init
	WORD version = MAKEWORD(2, 2);
	WSADATA data;

	if (WSAStartup(version, &data) != 0) {
		std::cerr << "WSAStartup() failure" << std::endl;
		return -1;
	}
#endif /* _WIN32 */

	//! Enable logging
	cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	cpp_redis::client client;

	// for Conan testing we just returning here as only linking is needed to be verified.
	return 0;

	client.connect("127.0.0.1", 6379,
	               [](const std::string &host, std::size_t port, cpp_redis::connect_state status) {
			               if (status == cpp_redis::connect_state::dropped) {
				               std::cout << "client disconnected from " << host << ":" << port << std::endl;
			               }
	               });

	auto replcmd = [](const cpp_redis::reply &reply) {
			std::cout << "set hello 42: " << reply << std::endl;
			// if (reply.is_string())
			//   do_something_with_string(reply.as_string())
	};

	const std::string group_name = "groupone";
	const std::string session_name = "sessone";
	const std::string consumer_name = "ABCD";

	std::multimap<std::string, std::string> ins;
	ins.insert(std::pair<std::string, std::string>{"message", "hello"});

#ifdef ENABLE_SESSION

	client.xadd(session_name, "*", ins, replcmd);
	client.xgroup_create(session_name, group_name, replcmd);

	client.sync_commit();

	client.xrange(session_name, {"-", "+", 10}, replcmd);

	client.xreadgroup({group_name,
	                   consumer_name,
	                   {{session_name}, {">"}},
	                   1, // Count
	                   0, // block milli
	                   false, // no ack
	                  }, [](cpp_redis::reply &reply) {
			std::cout << "set hello 42: " << reply << std::endl;
			auto msg = reply.as_array();
			std::cout << "Mes: " << msg[0] << std::endl;
			// if (reply.is_string())
			//   do_something_with_string(reply.as_string())
	});

#else

	// same as client.send({ "SET", "hello", "42" }, ...)
	client.set("hello", "42", [](cpp_redis::reply &reply) {
			std::cout << "set hello 42: " << reply << std::endl;
			// if (reply.is_string())
			//   do_something_with_string(reply.as_string())
	});

	// same as client.send({ "DECRBY", "hello", 12 }, ...)
	client.decrby("hello", 12, [](cpp_redis::reply &reply) {
			std::cout << "decrby hello 12: " << reply << std::endl;
			// if (reply.is_integer())
			//   do_something_with_integer(reply.as_integer())
	});

	// same as client.send({ "GET", "hello" }, ...)
	client.get("hello", [](cpp_redis::reply &reply) {
			std::cout << "get hello: " << reply << std::endl;
			// if (reply.is_string())
			//   do_something_with_string(reply.as_string())
	});

#endif

	// commands are pipelined and only sent when client.commit() is called
	// client.commit();

	// synchronous commit, no timeout
	client.sync_commit();

	// synchronous commit, timeout
	// client.sync_commit(std::chrono::milliseconds(100));

#ifdef _WIN32
	WSACleanup();
#endif /* _WIN32 */

	return 0;
}
