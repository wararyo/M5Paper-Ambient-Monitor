// nscanf by h-nari https://github.com/h-nari/nscanf

#ifdef __cplusplus
extern "C" {
#endif

int nscanf(const char *str, const char *fmt, ...)
  __attribute__ ((format (scanf, 2, 3)));

#ifdef __cplusplus
}
#endif
