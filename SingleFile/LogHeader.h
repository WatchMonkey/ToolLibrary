/*
 * LogHeader.h
 *
 *  Created on: 2021年4月19日
 *      Author: 14452
 */

#ifndef INCLUDE_LOGMACRO_H_
#define INCLUDE_LOGMACRO_H_


#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>


//! 日志文件
static FILE* Log_File = NULL;
//! 输出到stdout中
static bool Send_stdout = false;
//!	显示日期
static bool Contain_Date = false;
//! 日志信息级别
enum Info_Level{f_Info = 0x10,f_Warning = 0x12,f_Error = 0x14};
typedef enum Info_Level Info_Level_t;
static Info_Level_t Log_level = f_Info;
//! 互斥量
static pthread_mutex_t Log_localmutex;
static pthread_mutex_t Log_datamutex;

static unsigned int temp_init_success = 0x0;
static bool temp_init_status = true;
//!	日志缓存
static char* temp_log_buffer = NULL;

/*!
    @brief  设置使用的日志文件
    @param	filepath:日志文件
    @retval 0x0-success
    		other-failed
	@return int
*/
int InitializeLog(char* filepath)
{
    if(pthread_mutex_init(&Log_localmutex,NULL) == 0x0 || pthread_mutex_init(&Log_datamutex,NULL) == 0x0){
        temp_init_success |= 0x1;
    }else{
    	temp_init_status = false;
    	return 0x1;
    }

    temp_log_buffer = (char*)calloc(1024,0x1);
    if(temp_log_buffer != NULL){
        temp_init_success |= 0x2;
    }else{
    	temp_init_status = false;
    	return 0x1;
    }

    //open file
    Log_File = fopen(filepath,"w+");
    if(Log_File != NULL){
    	temp_init_success |= 0x4;
    }else{
    	temp_init_status = false;
    	return 0x1;
    }

    temp_init_status = true;
    return 0x0;
}

/*!
 * 	@brief	释放
 * 	@retval	void
 */
void ReleaseLog()
{
	if(temp_init_success == 0x0){
		return;
	}

	if(temp_init_success & 0xFFFFFFFE){
		pthread_mutex_destroy(&Log_localmutex);
		pthread_mutex_destroy(&Log_datamutex);
	}
	if(temp_init_success & 0xFFFFFFFD){
		free(temp_log_buffer);
	}
	if(temp_init_success & 0xFFFFFFFB){
		fflush(Log_File);
		fclose(Log_File);
	}
}

/*!
    @brief  设置信息是否输出到stdout中
    @param	value
    @retval void
*/
void SetSendStdout(bool value)
{
    pthread_mutex_lock(&Log_localmutex);
    Send_stdout = value;
    pthread_mutex_unlock(&Log_localmutex);
}

/*!
    @brief  设置日志信息筛选级别
    @param	筛选的日志级别
    @retval void
*/
void SetLogLevel(Info_Level_t value)
{
    pthread_mutex_lock(&Log_localmutex);
    Log_level = value;
    pthread_mutex_unlock(&Log_localmutex);
}

/*!
    @brief  设置日志显示日期
    @param	筛选的日志级别
    @retval void
*/
void SetLogDate(bool value)
{
    pthread_mutex_lock(&Log_localmutex);
    Contain_Date = value;
    pthread_mutex_unlock(&Log_localmutex);
}

/*!
    @brief  日志信息输出
    @param	value:日志级别
    @param	line:日志所在行数
    @param	function:日志所在函数
    @param	file:日志所在文件
    @param	format
    @param	变长参数
    @retval	void
*/
void LogPrint(Info_Level_t level,int line,const char* function,const char* file,const char* format,...)
{
	if(temp_init_status == false){
		return;
	}

	// detected the level
	pthread_mutex_lock(&Log_localmutex);
	Info_Level_t temp_level = Log_level;
	pthread_mutex_unlock(&Log_localmutex);
	if((int)level < (int)temp_level)return;

	pthread_mutex_lock(&Log_localmutex);
	bool temp_stdout = Send_stdout;
	bool temp_date = Contain_Date;
	pthread_mutex_unlock(&Log_localmutex);

	//时间输出
	struct tm today;
	time_t now;
	now = time(NULL);
	//now = now + 28880; //因为我们在+8时区 后面要用gmtime分解时间 必须手工修正一下 就是60秒*60分钟*8小时 = 28880
	gmtime_r(&now, &today);
	int y,m,d,h,n,s;
	y = today.tm_year+1900;
	m = today.tm_mon+1;
	d = today.tm_mday;
	h = today.tm_hour;
	n = today.tm_min;
	s = today.tm_sec;

	pthread_mutex_lock(&Log_datamutex);
	if(temp_date){
		fprintf(Log_File,"(%d-%02d-%02d)",y,m,d);
		if(level == f_Error || temp_stdout == true){
			fprintf(stdout,"(%d-%02d-%02d)",y,m,d);
		}
	}
	fprintf(Log_File,"(%02d.%02d.%02d)",h,n,s);
	if(level == f_Error || temp_stdout == true){
		fprintf(stdout,"(%02d.%02d.%02d)",h,n,s);
	}

	//信息级别输出
	switch(level)
	{
	case f_Info:
		fprintf(Log_File,"[Information]");
		if(temp_stdout){
			fprintf(stdout,"[Information]");
		}
		break;
	case f_Warning:
		fprintf(Log_File,"[Warning]");
		if(temp_stdout){
			fprintf(stdout,"[Warning]");
		}
		break;
	case f_Error:
		fprintf(Log_File,"[Error]");
		fprintf(stdout,"[Error]");
		break;
	default:
		fprintf(Log_File,"[Unknown]");
		if(temp_stdout){
			fprintf(stdout,"[Unknown]");
		}
		break;
	}

	//日志信息输出
	va_list temp_va;
	va_start(temp_va,format);
	vfprintf(Log_File,format,temp_va);
	if(level == f_Error || temp_stdout == true){
		vfprintf(stdout,format,temp_va);
	}
	va_end(temp_va);
	
	//文件信息输出
	fprintf(Log_File," <<<%s:%d:%s>",function,line,file);
	if(level == f_Error || temp_stdout == true){
		fprintf(stdout," <<<%s:%d:%s>",function,line,file);
	}
	
	//换行输出
	fprintf(Log_File,"\n");
	if(level == f_Error || temp_stdout == true){
		fprintf(stdout,"\n");
		fflush(stdout);
	}

	pthread_mutex_unlock(&Log_datamutex);
}


//! Log Macro
#define ILog(format,...) LogPrint(f_Info,__LINE__,__FUNCTION__,__FILE__,format,##__VA_ARGS__)
#define WLog(format,...) LogPrint(f_Warning,__LINE__,__FUNCTION__,__FILE__,format,##__VA_ARGS__)
#define ELog(format,...) LogPrint(f_Error,__LINE__,__FUNCTION__,__FILE__,format,##__VA_ARGS__)

#endif /* INCLUDE_LOGMACRO_H_ */
