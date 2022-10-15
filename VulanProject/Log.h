#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <memory>

class Log
{
public:
	static void init();
	inline static std::shared_ptr<spdlog::logger>& getLogger()
	{
		return s_coreLogger;
	}
private:
	static std::shared_ptr<spdlog::logger> s_coreLogger;
};

#ifdef NDEBUG
#define VULKAN_CORE_ERROR(...)
#define VULKAN_CORE_WARN(...)
#define VULKAN_CORE_INFO(...)
#define VULKAN_CORE_TRACE(...)
#else
#define VULKAN_CORE_ERROR(...) Log::getLogger()->error(__VA_ARGS__)
#define VULKAN_CORE_WARN(...) Log::getLogger()->warn(__VA_ARGS__)
#define VULKAN_CORE_INFO(...) Log::getLogger()->info(__VA_ARGS__)
#define VULKAN_CORE_TRACE(...) Log::getLogger()->trace(__VA_ARGS__)
#endif