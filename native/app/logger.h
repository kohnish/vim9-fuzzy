
void _logger(const char *msg, const char *level, int line_num, const char *filename);
#define LOG_ERROR(msg) _logger(msg, "error", __LINE__, __FILE__);
