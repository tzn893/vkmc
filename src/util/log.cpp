#include "log.h"

std::shared_ptr<spdlog::logger> _al_logger;

struct LoggerInitializeHelper
{
	LoggerInitializeHelper()
	{
		_al_logger = spdlog::stdout_color_mt("vkmc");
		_al_logger->set_level(spdlog::level::trace); 
		_al_logger->set_pattern("%^[%T] %n: %v%$");
	}

	~LoggerInitializeHelper()
	{
		_al_logger = nullptr;
	}
};


static LoggerInitializeHelper _logger_helper;