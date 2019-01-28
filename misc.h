#pragma once

#include <stdint.h>

uint64_t TimestampUs();
void Sleep(int ms);
void SleepUs(int us);
void DieErrno(const char *s);
void Die(const char *s);
void RegisterSignals(void (*signal_handler)(int) );
void SetStandardInputNonBlocking();
bool IsStandardInputEOF();

