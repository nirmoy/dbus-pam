void log_print(FILE*fp, char* filename, int line, char *fmt,...);

#define log(...) log_print(stdout, __FILE__, __LINE__, __VA_ARGS__ )
#define log_err(...) log_print(stderr, __FILE__, __LINE__, __VA_ARGS__ )

#ifdef DBG
    #define log_dbg log
#else
    #define log_dbg(...) 
#endif

/* log_file() TODO
 * 
 * 
 * */
