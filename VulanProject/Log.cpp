#include "Log.h"

std::shared_ptr<spdlog::logger> Log::s_coreLogger;

void Log::init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");
	s_coreLogger = spdlog::stdout_color_mt("Vulkan");
	s_coreLogger->set_level(spdlog::level::trace);
}
