#ifdef __cplusplus
extern "C" {
#endif

void beep_buzzer_shutdown(int pin);
int beep_buzzer(int pin, int frequencyHz, int durationMs);

#ifdef __cplusplus
}
#endif