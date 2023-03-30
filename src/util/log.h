#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

extern std::shared_ptr<spdlog::logger> _al_logger;

#define vkmc_log(msg,...) _al_logger->info(msg,__VA_ARGS__)
#define vkmc_error(msg,...) _al_logger->error(msg,__VA_ARGS__);

