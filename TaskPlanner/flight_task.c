/**
 * @file flight_task.c
 * @author a-styuf (a-styuf@yandex.ru)
 * @brief полетные задания для работы со стандартными циклограммами планировщика задач
 * @version 0.1
 * @date 2023-03-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "flight_task.h"

/**
 * @brief инициализация полетного задания
 * 
 * @param ft_ptr 
 * @return int8_t 1 - успешная инициализация, 0 - ошибка
 */
int8_t ft_init(typeFT *ft_ptr, char* name, uint8_t self_num, uint16_t sat_id, uint8_t dev_id)
{
    //изначальное обнуление параметров
    memset((uint8_t *)ft_ptr, 0x00, sizeof(typeFT));
    //
    strcpy(ft_ptr->name, name);
    //
    ft_ptr->self_num = self_num;
    ft_ptr->sat_id = sat_id;
    ft_ptr->dev_id = dev_id;
    ft_ptr->print_ena = 1;
    //
    ft_reset_status(ft_ptr);
    return 1;
}

/**
 * @brief сброс статуса и ошибок полетного задания
 * 
 * @param ft_ptr 
 */
void ft_reset_status(typeFT *ft_ptr)
{
    ft_status_collector(ft_ptr, FT_STATUS_CLEAR, FT_STATUS_SET);
    ft_ptr->start_cnter = 0;
    ft_ptr->error_cnt = 0;
}

/**
	* @brief  функция для запуска в планировщике задач обработки светодиодов
	* @param  ctrl_struct указатель на программную модель устройства
	* @param  time_us глобальное время
	* @param  interface interface из процесса на верх
  */
int8_t ft_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	int8_t retval = 0;
	typeFT* ft_ptr = (typeFT*)ctrl_struct;
	ft_process(ft_ptr, time_us/1000);
	return retval;
}

/**
 * @brief обработка шага полетного задания
 * 
 * @param ft_ptr 
 * @param time_ms 
 * @return int8_t 
 */
int8_t ft_process(typeFT *ft_ptr, uint64_t time_ms)
{   
    ft_ptr->last_call_period_ms = time_ms - ft_ptr->last_call_time_ms;
    ft_ptr->last_call_time_ms = time_ms;
    // обработка таймеров
    for (uint8_t tim_num = 0; tim_num < 8; tim_num++){
        if (ft_ptr->timer[tim_num].status){
            ft_ptr->timer[tim_num].cnter += ft_ptr->last_call_period_ms; 
            if (ft_ptr->timer[tim_num].cnter >= ft_ptr->timer[tim_num].bnd_val){
                ft_ptr->timer[tim_num].status = 0;
                ft_ptr->timer[tim_num].cnter = 0;
                ft_ptr->timer[tim_num].bnd_val = 0;
            }
        }
    }
    // обработка состояний полетного задания
    switch(ft_ptr->mode){
        case FT_MODE_OFF:
            //
            break;
        case FT_MODE_START:
                ft_set_mode(ft_ptr, FT_MODE_WORK);
                ft_ptr->ctrl.step_delay = 0; //для перехода к новому шагу
                ft_ptr->ctrl.rpt_value = 0; //для перехода к новому шагу
                ft_ptr->ctrl.last_step_time = time_ms;
                ft_refresh_go_to_parameters(ft_ptr);
                //
                ft_ptr->start_cnter++;
            break;
        case FT_MODE_WORK:
            ft_mode_work_process(ft_ptr, time_ms);
            break;
        case FT_MODE_PAUSE:
            ft_ptr->ctrl.pause_time += time_ms - ft_ptr->ctrl.last_pause_time;
            ft_ptr->ctrl.last_pause_time = time_ms;
            break;
    }
    //
    if(ft_ptr->mode == FT_MODE_WORK) ft_status_collector(ft_ptr, FT_STATUS_WORK, FT_STATUS_SET);
    else ft_status_collector(ft_ptr, FT_STATUS_WORK, FT_STATUS_RELEASE);
    //
    if(ft_ptr->mode == FT_MODE_PAUSE) ft_status_collector(ft_ptr, FT_STATUS_PAUSE, FT_STATUS_SET);
    else ft_status_collector(ft_ptr, FT_STATUS_PAUSE, FT_STATUS_RELEASE);
    //
    return ft_ptr->mode;
}

/**
 * @brief Установка режима работы полетного задания с обработкой корректности управления
 * 
 * @param ft_ptr указатель на структуру управления полетным заданием
 * @param mode режим работы полетного задания
 * @return int8_t -1 - некорректный режим, 0 - не удалось установить режим, так как это невозможно, 1 - режим успешно установлен
 */
int8_t ft_set_mode(typeFT *ft_ptr, uint8_t mode)
{
    if(ft_ptr->print_ena) printf("ft_<%d>_mode_set:", ft_ptr->self_num);
    switch(mode){
        case FT_MODE_OFF:
            __ft_step_cast_reset(ft_ptr);
            ft_ptr->mode = mode;
            if(ft_ptr->print_ena) printf("<off>\n");
            return 1;
        case FT_MODE_WORK:
            if(ft_ptr->mode == FT_MODE_WORK) {}
            else if(ft_ptr->mode == FT_MODE_START) {
                ft_ptr->mode = FT_MODE_WORK;
                if(ft_ptr->print_ena) printf("<work>\n");
                return 1;
            }
            else if(ft_ptr->mode == FT_MODE_PAUSE){
                ft_ptr->ctrl.last_step_time += ft_ptr->ctrl.pause_time;
                ft_ptr->ctrl.pause_time = 0;
                ft_ptr->mode = FT_MODE_WORK;
                if(ft_ptr->print_ena) printf("<work>\n");
                return 1;
            }
            break;
        case FT_MODE_START:
            ft_ptr->mode = FT_MODE_START;
            if(ft_ptr->print_ena) printf("<start>\n");
            return 1;
        case FT_MODE_PAUSE:
            if (ft_ptr->mode == FT_MODE_WORK){
                ft_ptr->ctrl.pause_time = 0;
                ft_ptr->mode = FT_MODE_PAUSE;
                if(ft_ptr->print_ena) printf("<pause>\n");
                return 1;
            }
            else {}
            break;
        default:
            if(ft_ptr->print_ena) printf("<error>\n");
            ft_status_collector(ft_ptr, FT_STATUS_OTHER_ERROR, FT_STATUS_SET);
            return -1;
    }
    if(ft_ptr->print_ena) printf("<skip>\n");
    return 0;
}

/**
 * @brief проверка, работает ли сейчас полетное задание
 * 
 * @param ft_ptr 
 * @return uint8_t 1 - работает, 0 - работает
 */
uint8_t ft_get_operation_status(typeFT *ft_ptr)
{
    return (ft_ptr->mode != FT_MODE_OFF) ? 1 : 0;
}

/**
 * @brief проверка, усть лои у ПЗ ошибка
 * 
 * @param ft_ptr 
 * @return uint8_t 1 - ошибка, 0 - нет ошибки
 */
uint8_t ft_get_error_status(typeFT *ft_ptr)
{
    if (ft_ptr->status & (  FT_STATUS_OTHER_ERROR |
                            FT_STATUS_STEP_ERROR |
                            FT_STATUS_CRC_ERROR |
                            FT_STATUS_LUNCH_ERROR |
                            FT_STATUS_WR_TO_ROM_ERROR |
                            FT_STATUS_WR_TO_RAM_ERROR |
                            FT_STATUS_HEADER_ERROR |
                            FT_STATUS_FUNC_ERROR |
                            FT_STATUS_FIFO_ERROR)) {
        return 1;
    }
    else{
        return 0;
    }
    
}

void ft_mode_work_process(typeFT *ft_ptr, uint64_t time_ms)
{   
    uint8_t step_num = 0;
    uint8_t last_run_flag = 0;
    // проверка на необходимость выполнения шага
    if ((time_ms - ft_ptr->ctrl.last_step_time) >= ft_ptr->ctrl.step_delay) {  // достижение условия окончания паузы до следующего вызова
        ft_ptr->ctrl.last_step_time = time_ms;
    }
    else return;
    // создание слепка функции шага только при нулевом значении rpt_value
    if (ft_ptr->ctrl.rpt_value == 0) {
			ft_create_step_cast(ft_ptr);
			last_run_flag = 1;
		}
    // запуск функции из слепка
    ft_ptr->ctrl.func_ret_val = ft_func_run(ft_ptr);
    // проверка возвращаемого значения
    ft_function_retval_check(ft_ptr);
    // контроль повторных вызовов шага
    if ((ft_ptr->ctrl.rpt_value == 0) || (last_run_flag)) {
        if (ft_ptr->ctrl.go_to_cnt[ft_ptr->ctrl.step_num] == 0){
            ft_ptr->ctrl.step_num++;
            // сборка информации о количествах переходов goto
            for(step_num=0; step_num < ft_ptr->ctrl.step_num; step_num++){
                ft_ptr->ctrl.go_to_cnt[step_num] = ft_ptr->ctrl.go_to_max[step_num];
            }
        }
        else{
            ft_ptr->ctrl.go_to_cnt[ft_ptr->ctrl.step_num]--;
            ft_ptr->ctrl.step_num = ft_ptr->ctrl.step_cast.fields.go_to;
        }
    }
    else {
        ft_ptr->ctrl.rpt_value--;
    }
    //
    if (ft_ptr->ctrl.step_num > FT_LEN_STEP){
        ft_set_mode(ft_ptr, FT_MODE_OFF);
    }
}

/**
 * @brief создание слепка шага полетного задания по номеру шага
 * 
 * @param ft_ptr 
 * @return int8_t 1 - слепок создан, 0 - слепок не может быть создан
 */
int8_t ft_create_step_cast(typeFT *ft_ptr)
{
    memset((uint8_t*)&ft_ptr->ctrl.step_cast, 0x00, sizeof(typeFTStep));
    //
    if (ft_ptr->ctrl.step_num < FT_LEN_STEP) {
        ft_ptr->ctrl.step_cast = ft_ptr->task.step[ft_ptr->ctrl.step_num]; // полная копия шага для работы
        ft_ptr->ctrl.rpt_value = ft_ptr->ctrl.step_cast.fields.rpt_cnt;
        ft_ptr->ctrl.step_delay = ft_ptr->ctrl.step_cast.fields.delay_ms;
        if(ft_ptr->ctrl.step_cast.fields.label != FT_MARK) {
            return 0;
        }
        else if (ft_check_step_cast_crc(ft_ptr) == 0) {
            ft_status_collector(ft_ptr, FT_STATUS_CRC_ERROR, FT_STATUS_SET);
            return 0;
        }
        return 1;
    }
    else{
        ft_ptr->ctrl.rpt_value = 0;
        ft_ptr->ctrl.step_delay = 0;
        return 0;
    }
}

/**
 * @brief функция запуска функции полетного задания
 * 
 * @param ft_ptr 
 * @return int32_t 
 */
int32_t ft_func_run(typeFT* ft_ptr)
{
    uint32_t type, cmd;
    // подготовка переменных
    type = ft_ptr->ctrl.step_cast.fields.type;
    cmd = ft_ptr->ctrl.step_cast.fields.cmd;
    memset(ft_ptr->ctrl.data_to_save, 0xFE, sizeof(ft_ptr->ctrl.data_to_save)); 
    ft_ptr->ctrl.data_len = 0;
    ft_ptr->ctrl.func_ret_val = 0;
    // проверка заголовка шага полетного задания
    if (ft_ptr->ctrl.step_cast.fields.label != FT_MARK){
        ft_ptr->ctrl.func_ret_val = 0;
        return ft_ptr->ctrl.func_ret_val;
    }
    // запуск функции
    if (ft_ptr->f_group[type].func[cmd].func != NULL){
        ft_ptr->ctrl.func_ret_val = ft_ptr->f_group[type].func[cmd].func(   ft_ptr,
                                                                            ft_ptr->f_group[type].func[cmd].ctrl_struct, 
                                                                            ft_ptr->ctrl.step_cast.fields.data,
                                                                            ft_ptr->ctrl.data_to_save, 
                                                                            &ft_ptr->ctrl.data_len);
        
        //  
        if(ft_ptr->print_ena) printf("ft_<%d> st_<%d> func: t<%ld>c<%ld>ret<%ld>, data: ", ft_ptr->self_num, ft_ptr->ctrl.step_num, (long)type, (long)cmd, (long)ft_ptr->ctrl.func_ret_val);
        if(ft_ptr->print_ena) printf("\n");
    }
    else{
        ft_ptr->ctrl.func_ret_val = 0;
    }
    return ft_ptr->ctrl.func_ret_val;
}

int32_t ft_function_retval_check(typeFT* ft_ptr)
{
    if(ft_ptr->ctrl.func_ret_val & FT_STATUS_BRANCH_RPT_BREAK){
        if(ft_ptr->ctrl.step_cast.fields.settings & FT_STATUS_BRANCH_RPT_BREAK){
            ft_ptr->ctrl.rpt_value = 0;
            if(ft_ptr->print_ena) printf("ft_<%d> rpt break\n", ft_ptr->self_num);
        }
    }
    if (ft_ptr->ctrl.func_ret_val & FT_STATUS_BRANCH_FT_BREAK){
        if(ft_ptr->ctrl.step_cast.fields.settings & FT_STATUS_BRANCH_FT_BREAK){
            ft_ptr->ctrl.step_num = FT_LEN_STEP;
            if(ft_ptr->print_ena) printf("ft_<%d> ft break\n", ft_ptr->self_num);
        }
    }
    if (ft_ptr->ctrl.func_ret_val & FT_STATUS_BRANCH_GO_TO_INH){
        if(ft_ptr->ctrl.step_cast.fields.settings & FT_STATUS_BRANCH_GO_TO_INH){
            ft_ptr->ctrl.go_to_cnt[ft_ptr->ctrl.step_num] = 0;
            if(ft_ptr->print_ena) printf("ft_<%d> ft go_to dis\n", ft_ptr->self_num);
        }
    }
    if (ft_ptr->ctrl.func_ret_val & FT_STATUS_SAVE_DATA_ENA){
        if(ft_ptr->ctrl.step_cast.fields.settings & FT_STATUS_SAVE_DATA_ENA){
            ft_write_fifo(ft_ptr, (uint8_t*)&ft_ptr->ctrl.data_to_save);
            ft_ptr->ctrl.save_cnt++;
            memset(ft_ptr->ctrl.data_to_save, 0xFE, sizeof(ft_ptr->ctrl.data_to_save));    
            if(ft_ptr->print_ena) printf("ft_<%d> ft save raw data\n", ft_ptr->self_num);
        }
    }
    return 0;
}

/**
 * @brief Проверка контрольной суммы отдельного шага
 * 
 * @param ft_ptr 
 * @return int8_t 
 */
int8_t ft_check_step_cast_crc(typeFT *ft_ptr)
{
    uint16_t crc16 = ft_crc16(ft_ptr->ctrl.step_cast.array, sizeof(typeFTStep) - 2);
    if (crc16 == ft_ptr->ctrl.step_cast.fields.crc16) {
        return 1;
    }
    ft_status_collector(ft_ptr, FT_STATUS_CRC_ERROR, FT_STATUS_SET);
    return 0;
}

/**
 * @brief Приведение переменных управления циклограммой к начальному значению
 * 
 * @param ft_ptr 
 */
void __ft_step_cast_reset(typeFT *ft_ptr)
{
    memset(&ft_ptr->ctrl, 0x00, sizeof(typeFTCtrl));
}


/**
 * @brief Регистрация функции для полетного задания
 * 
 * @param ft_ptr 
 * @param group_num Номер группы функций
 * @param func_num номер функции в группе
 * @param func указатель на функцию
 * @param ctrl_struct управляющая структура для регистрируемой функции
 * @return int8_t 1- функция зарегистрирована, 0 - функция не зарегистрирована
 */
int8_t ft_function_registration(typeFT *ft_ptr, uint8_t group_num, uint8_t func_num, int32_t (*func) (void*, void*, uint8_t*, uint8_t*, uint8_t*), void* ctrl_struct)
{
    if (group_num < FT_FUNC_GROUP_NUM_MAX){
        if (func_num < FT_FUNCT_NUM_MAX){
            ft_ptr->f_group[group_num].func[func_num].func        = func;
            ft_ptr->f_group[group_num].func[func_num].ctrl_struct = ctrl_struct;
            //if(ft_ptr->print_ena) printf("\t ft_func_reg: t <%d>, cmd <%d> \n", group_num, func_num);
            return 1;
        }
    }
    ft_status_collector(ft_ptr, FT_STATUS_FUNC_ERROR, FT_STATUS_SET);
    return 0;
}

/**
 * @brief Загрузка массива полетного задания (массив в little-endian)
 * 
 * @param ft_ptr 
 * @param task_array массив данных с полетным заданием
 * @return int8_t -1 - некорректный массив с данными, >= 0  - количество рабочих шагов в полетном задании
 */
int8_t ft_load_task(typeFT *ft_ptr, uint8_t* task_array)
{
	uint8_t step_num = 0, total_step_num = 0;
	uint16_t crc16;
    typeFTStep step;
    //
    memset((uint8_t*)ft_ptr->task.array, 0x00, sizeof(ft_ptr->task));
    //
    for (step_num = 0; step_num < FT_LEN_STEP; step_num++){
        memcpy((uint8_t*)&step, task_array+FT_STEP_LEN_BYTE*step_num, sizeof(typeFTStep));
        // проверка валидности шага
        if(step.fields.label == FT_MARK){
            crc16 = ft_crc16((uint8_t*)step.array, 62);
            if (step.fields.crc16 == crc16){
                total_step_num += 1;
            }
            else{
                ft_status_collector(ft_ptr, FT_STATUS_CRC_ERROR, FT_STATUS_SET);
                ft_status_collector(ft_ptr, FT_STATUS_WR_TO_RAM_ERROR, FT_STATUS_SET);
                return -1;
            }
        }
        else if((step.fields.label == 0x0000) && (step.fields.crc16 == 0x0000)){
            break;
        }
        else{
            ft_status_collector(ft_ptr, FT_STATUS_HEADER_ERROR, FT_STATUS_SET);
            ft_status_collector(ft_ptr, FT_STATUS_WR_TO_RAM_ERROR, FT_STATUS_SET);
            return -1;
        }
    }
    // копирование только правильных подсчитанных шагов, если не было проблем с заголовком и crc16
    memcpy((uint8_t*)ft_ptr->task.array, task_array, total_step_num*FT_STEP_LEN_BYTE);
    //
    ft_refresh_go_to_parameters(ft_ptr);
    //
    return total_step_num;
}

/**
 * @brief Обновление параметров работы переходов goto
 * 
 * @param ft_ptr 
 */
void ft_refresh_go_to_parameters(typeFT *ft_ptr)
{
    uint8_t step_num = 0;
    // сборка информации о количествах переходов goto
    for(step_num=0; step_num < FT_LEN_STEP; step_num++){
        ft_ptr->ctrl.go_to_cnt[step_num] = ft_ptr->task.step[step_num].fields.go_cnt;
        ft_ptr->ctrl.go_to_max[step_num] = ft_ptr->task.step[step_num].fields.go_cnt;
    }
}

/**
 * @brief Выгрузка массива полетного задания
 * 
 * @param ft_ptr 
 * @return typeFTArray 
 */
typeFTArray ft_unload_task(typeFT *ft_ptr)
{
	return ft_ptr->task;
}


/**
 * @brief проверка полетного задания
 * 
 * @param ft_ptr 
 * @return int8_t -1 - некорректный массив с данными, 0  - количество рабочих шагов в полетном задании, > 0 - количество шагов
 */
int8_t ft_check_task(typeFT *ft_ptr)
{
	uint8_t step_num = 0, total_step_num = 0;
	uint16_t crc16;
    typeFTStep step;
    //
    for (step_num = 0; step_num < FT_LEN_STEP; step_num++){
        step = ft_ptr->task.step[step_num];
        // проверка валидности шага
        if(step.fields.label == FT_MARK){
            crc16 = ft_crc16((uint8_t*)step.array, 62);
            if (step.fields.crc16 == crc16){
                total_step_num += 1;
            }
            else{
                ft_status_collector(ft_ptr, FT_STATUS_CRC_ERROR, FT_STATUS_SET);
                ft_status_collector(ft_ptr, FT_STATUS_WR_TO_RAM_ERROR, FT_STATUS_SET);
                return -1;
            }
        }
        else if((step.fields.label == 0x0000) && (step.fields.crc16 == 0x0000)){
            break;
        }
        else{
            ft_status_collector(ft_ptr, FT_STATUS_HEADER_ERROR, FT_STATUS_SET);
            ft_status_collector(ft_ptr, FT_STATUS_WR_TO_RAM_ERROR, FT_STATUS_SET);
            return -1;
        }
    }
    // сборка информации о количествах переходов goto
    return total_step_num;
}

/**
  * @brief  запись измерения в fifo работаем с массивами по 128Б
	* @param  ft_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t ft_write_fifo(typeFT *ft_ptr, uint8_t* data)
{
	if (ft_ptr->fifo.rec_num >= FT_FIFO_DEPTH){
        ft_status_collector(ft_ptr, FT_STATUS_FIFO_ERROR, FT_STATUS_SET);
		return -1;
	}
	else{
		memcpy((uint8_t*)&ft_ptr->fifo.array[ft_ptr->fifo.rec_num][0], data, FT_FIFO_REC_LEN);
		ft_ptr->fifo.rec_num += 1;
		if (ft_ptr->fifo.rec_num > ft_ptr->fifo.rec_max) ft_ptr->fifo.rec_max = ft_ptr->fifo.rec_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

/**
  * @brief  взятие измерения из fifo работаем с массивами по 128Б
	* @param  ft_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t ft_read_fifo(typeFT* ft_ptr, uint8_t* data)
{
	if (ft_ptr->fifo.rec_num == 0){
		return 0;
	}
	else{
		ft_ptr->fifo.rec_num -= 1;
		memcpy((uint8_t*)data, &ft_ptr->fifo.array[0][0], FT_FIFO_REC_LEN);
		memmove(&ft_ptr->fifo.array[0][0], &ft_ptr->fifo.array[1][0], FT_FIFO_REC_LEN*ft_ptr->fifo.rec_num);
		memset((uint8_t*)&ft_ptr->fifo.array[ft_ptr->fifo.rec_num][0], 0x00, FT_FIFO_REC_LEN*(FT_FIFO_DEPTH - ft_ptr->fifo.rec_num));
		return 1;
	}
}

/**
 * @brief Создание отчета по работе ПЗ размером 14 байт
 * 
 * @param ft_ptr указатель на структуру управления полетным заданием
 * @return typeFTReport отчет по работе ПЗ
 */
typeFTReport ft_create_report(typeFT* ft_ptr)
{
    ft_ptr->report.fields.self_id = ft_ptr->self_num;
    ft_ptr->report.fields.mode = ft_ptr->mode;
    ft_ptr->report.fields.step_num = ft_ptr->ctrl.step_num;
    ft_ptr->report.fields.function_type = ft_ptr->ctrl.step_cast.fields.type & 0xFF;
    ft_ptr->report.fields.function_cmd = ft_ptr->ctrl.step_cast.fields.cmd & 0xFF;
    ft_ptr->report.fields.rpt_value = ft_ptr->ctrl.rpt_value;
    ft_ptr->report.fields.save_cnt = ft_ptr->ctrl.save_cnt;
    ft_ptr->report.fields.status = ft_ptr->status;
    ft_ptr->report.fields.err_cnter = ft_ptr->error_cnt;
    ft_ptr->report.fields.lunch_cnter = ft_ptr->start_cnter;
    //
    return ft_ptr->report;
}

/**
 * @brief Создание короткого отчета по работе ПЗ размером 4 байт
 * 
 * @param ft_ptr указатель на структуру управления полетным заданием
 * @return typeFTReport отчет по работе ПЗ
 */
typeFT_Short_Report ft_create_short_report(typeFT* ft_ptr)
{
    ft_ptr->short_report.fields.mode = ft_ptr->mode;
    ft_ptr->short_report.fields.step_num = ft_ptr->ctrl.step_num;
    ft_ptr->short_report.fields.function_type = ft_ptr->ctrl.step_cast.fields.type & 0xFF;
    ft_ptr->short_report.fields.function_cmd = ft_ptr->ctrl.step_cast.fields.cmd & 0xFF;
    //
    return ft_ptr->short_report;
}

/**
 * @brief Установка режима вывода отладочной информации
 * 
 * @param ft_ptr 
 * @param mode 0 - не выводится, >0 - выводиться
 */
void ft_set_print_mode(typeFT *ft_ptr, uint8_t mode)
{
    ft_ptr->print_ena = (mode) ? 1 : 0;
}

/**
 * @brief функция формирования шага полетного задания
 * 
 * @param step указатель на структуру шага
 * @param type тип команды
 * @param cmd номер команды
 * @param rpt_cnt счетчик повторов шага
 * @param delay_ms пауза до следующего шага
 * @param settings настройка работы (пока не используется)
 * @param data данные для функции
 */
void ft_create_ft_step(typeFTStep* step, uint16_t type, uint16_t cmd, uint16_t rpt_cnt, uint32_t delay_ms, uint16_t settings, uint16_t go_to, uint16_t go_cnt, uint8_t*data)
{
    step->fields.label      = FT_MARK;
    step->fields.type       = type;
    step->fields.cmd        = cmd;
    step->fields.rpt_cnt    = rpt_cnt;
    step->fields.go_to      = go_to;
    step->fields.go_cnt     = go_cnt;
    step->fields.delay_ms   = delay_ms;
    step->fields.settings   = settings;
    memset(step->fields.reserve, 0x00, sizeof(step->fields.reserve));
    memcpy(step->fields.data, data, sizeof(step->fields.data));
    step->fields.crc16      = ft_crc16(step->array, sizeof(typeFTStep) - 2);
    //if(ft_ptr->print_ena) printf("\t ft_create_step: f_t <%d> cmd <%d>, rpt <%d>, pause <%d>\n", type, cmd, rpt_cnt, delay_ms);
}

//
int32_t ft_def_fun_do_nothing(void* ft_ptr,  void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len)
{
	return FT_FUN_RET_NOPE;
}

int32_t ft_def_fun_start_timer_with_bnd(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len)
{
    typeFT* ft_ptr_internal = (typeFT*)ft_ptr;
    uint8_t tim_num = 0;
    //
    tim_num = *(uint16_t*)&ctrl_data[0];
    if (tim_num < 8){
        ft_ptr_internal->timer[tim_num].bnd_val = *(uint32_t*)&ctrl_data[2]; 
        ft_ptr_internal->timer[tim_num].status = 1;
    }
    return FT_FUN_RET_NOPE;
}

int32_t ft_def_fun_check_timer(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len)
{
    typeFT* ft_ptr_internal = (typeFT*)ctrl_struct;
    uint8_t tim_num = 0;
    //
    tim_num = *(uint16_t*)&ctrl_data[0];
    if (tim_num < 8){
        if (ft_ptr_internal->timer[tim_num].status) return 0;
        else return (FT_STATUS_BRANCH_RPT_BREAK | FT_STATUS_BRANCH_FT_BREAK | FT_STATUS_BRANCH_GO_TO_INH); 
    }
    return FT_FUN_RET_NOPE;
}

int32_t ft_def_fun_start_timer_without_bnd(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len)
{
    typeFT* ft_ptr_internal = (typeFT*)ctrl_struct;
    uint8_t tim_num = 0;
    //
    tim_num = *(uint16_t*)&ctrl_data[0];
    if (tim_num < 8){
        ft_ptr_internal->timer[tim_num].bnd_val = 0xFFFFFFFF; 
        ft_ptr_internal->timer[tim_num].status = 1;
    }
    return FT_FUN_RET_NOPE;
}

int32_t ft_def_fun_check_timer_by_ctrl_data(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len)
{
    typeFT* ft_ptr_internal = (typeFT*)ctrl_struct;
    uint8_t tim_num = 0;
    uint32_t bnd_val = *(uint32_t*)&ctrl_data[2];
    //
    tim_num = *(uint16_t*)&ctrl_data[0];
    if (tim_num < 8){
        // обработка таймеров
        if (ft_ptr_internal->timer[tim_num].status){
            if (ft_ptr_internal->timer[tim_num].cnter >= bnd_val){
                ft_ptr_internal->timer[tim_num].status = 0;
                ft_ptr_internal->timer[tim_num].cnter = 0;
                ft_ptr_internal->timer[tim_num].bnd_val = 0;
                return (FT_STATUS_BRANCH_RPT_BREAK | FT_STATUS_BRANCH_FT_BREAK | FT_STATUS_BRANCH_GO_TO_INH);
            }
        }
        else{
            return (FT_STATUS_BRANCH_RPT_BREAK | FT_STATUS_BRANCH_FT_BREAK | FT_STATUS_BRANCH_GO_TO_INH);
        }
    }
    return FT_FUN_RET_NOPE;
}

/**
 * @brief обработка статусов полезной нагрузки
 * 
 * @param pl_prdk_ptr 
 * @param flag флаг статуса
 * @param data 
 */
void  ft_status_collector(typeFT* ft_ptr, uint16_t flag, uint8_t type)
{
	if (flag == FT_STATUS_CLEAR) {
		ft_ptr->status = 0;
		return;
	}
	switch(type){
		case FT_STATUS_SET:
			ft_ptr->status |= flag;
			switch (flag){
				case FT_STATUS_OTHER_ERROR:
				case FT_STATUS_STEP_ERROR:
				case FT_STATUS_CRC_ERROR:
				case FT_STATUS_LUNCH_ERROR:
				case FT_STATUS_WR_TO_ROM_ERROR:
				case FT_STATUS_WR_TO_RAM_ERROR:
				case FT_STATUS_HEADER_ERROR:
				case FT_STATUS_FUNC_ERROR:
				case FT_STATUS_FIFO_ERROR:
                    ft_ptr->error_cnt++;
				break;
			}
			break;
		case FT_STATUS_RELEASE:
			ft_ptr->status &= ~(flag);
			break;
	}
}

//
//  CRC16 для полетного задания:
// 	Algorithm		Result		Check	Poly	Init	RefIn	RefOut	XorOut
//	CRC-16/MODBUS 	0x4B37		0x4B37	0x8005	0xFFFF	true	true	0x0000

unsigned short __ft_rev16(unsigned short data)
{
	return ((data >> 8) & 0xFF) + ((data & 0xFF) << 8);
}

static const uint16_t __ft_crc16tab[256]= 
{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
	};

/**
 * @brief CRC16 для полетного задания.
 * @brief Algorithm: CRC-16/MODBUS,	Result: 0x4B37, Check: 0x4B37, Poly: 0x8005, Init: 0xFFFF
 * @param buf 
 * @param len 
 * @return uint16_t 
 */
uint16_t ft_crc16(uint8_t *buf, uint8_t len)
{
	unsigned char i;
	unsigned short crc = 0xFFFF;
	for( i = 0; i < len; i++)
	{
		crc = (crc >> 8) ^ __ft_crc16tab[(crc ^ *(char *)(buf+i))&0xFF];
	}
	return __ft_rev16(crc);
}
