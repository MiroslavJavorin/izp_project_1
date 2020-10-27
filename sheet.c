
// DONE if function returns !0 call error and its error message
// DONE use only first seps ni info_init
// DONE check if num of seps in 1. line == nu of seps in other lines
// DONE add to parsing arguments if dcol n dcol n+1 call dcols n n+1
// DONE give functoins separators struct
// DONE make num_of_seps return exit_code
// DONE make all functoins return exit_code
// DONE make pointers all functoins arguments
// DONE change position+=2 to position+=3
// DONE comment, delete unused code from dcol
// DONE issue with separators. Calls segmentations fault
// TODO data processing parsing
// TODO make prsing
// TODO inplement data prpcessing functions
// TODO dpf_option сделать с помощью enum
// TODO check if in cset, beginswith and contains string is shorter than 100. Else return ERROR

//region Includes
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//endregion

//region defines
//region Errors
enum errors
{
    EMPTY_STDIN_ERROR = 2,
    MAX_LENGTH_REACHED,
    UNSUPPORTED_SEPARATORS_ERROR,
    WRONG_SEPARATORS_ERROR,
    WRONG_NUMBER_OF_SEPS_ERROR,
    NUMBER_OF_ROW_SELECTION_ERROR,
    TOO_MUCH_ARGUMENTS_ERROR,
    BAD_ARGUMENTS_ERROR_DCOL,
    BAD_ARGUMENTS_ERROR_DCOLS,
    BAD_ARGUMENTS_ERROR_DROW,
    BAD_ARGUMENTS_ERROR_DROWS,
    BAD_ARGUMENTS_ERROR_IROW,
    BAD_ARGUMENTS_ERROR_ICOL,
    COLUMN_OUT_OF_RANGE_ERROR,
    TOO_FEW_ARGS_AFTER_SELECTION,
    TOO_FEW_ARGS_AFTER_DPF, // selection <= 0 or is a letter
    DPF_IOOF_ERROR,
    SELECTION_IOOF_ERROR, //20
    TOO_FEW_ARGS_ERROR
};
//endregion

//region Lengths
#define MAX_ROW_LENGTH          1024
#define BUFFER_LENGTH           1024
#define CELL_LENGTH              100
#define SEPS_SIZE                127
#define MAX_NUMBER_OF_ARGUMENTS  100
//endregion

//region Sizes of arrays with functions
#define NULL_PARAM_SIZE_TABLE   2
#define ONE_PARAM_SIZE_TABLE    4
#define TWO_PARAMS_SIZE_TABLE   2
#define ONE_PARAMS_SIZE_DATA    4
#define TWO_PARAMS_SIZE_DATA    3
#define THREE_PARAMS_SIZE_DATA 17
//endregion

//region dpf_option
enum dpf_functions_enum
{
    NO_DPF,
    TOLOWER,
    TOUPPER,
    ROUND,
    INT,
    COPY,
    MOVE,
    SWAP,
    CSET
};

enum dpf_rows_selection_enum
{
    NO_SELECTION,
    ROWS,
    BEGINSWITH,
    CONTAINS
};
//endregion
//endregion


//region Structures
/**
 * Contains information about separators
 */
typedef struct seps_struct
{
    char separators[SEPS_SIZE];
    int number_of_seps;
}seps_struct;

/**
 * Contains informatoin about scanned row
 */
typedef struct row_info
{
    int i; // position of the last element of the row
    int current_row;
    char cache[MAX_ROW_LENGTH];
    seps_struct seps;
    seps_struct row_seps;
    int last_s[MAX_ROW_LENGTH];
    int num_of_cols;

    char buffer[MAX_ROW_LENGTH]; // for functoin swap
    /**
     * Array of 0 and 1, when 0 means there is no number in the column
     * Used for function csum
     */
    double sum;
    char cols_with_nums[MAX_ROW_LENGTH/2+1];
}row_info;

/**
 * Table edit functions sctruct
 */
typedef struct tef_struct
{
    unsigned int victims[MAX_ROW_LENGTH];
    unsigned int called;
}tef_struct;

typedef struct
{
    tef_struct d_col;
    tef_struct i_col;
    tef_struct a_col;
    tef_struct a_row;
    tef_struct i_row;
    tef_struct d_row;
}table_edit;

typedef struct row_selection
{
    /**
     * rs_option says which data processing function program will be using
     * 1 - cset; 2 - tolower; 3 - toupper; 4 - round; 5 - int; 6 - copy; 7 - swap; 8 - move;
     */
    char rs_option;
    int from;
    int to;
    char pattern[CELL_LENGTH];
}row_selection;

typedef struct
{
    char dpf_option; //cset tolower toupper...  1 2 3 4 ..
    int column1; // for functions cset, tolower, toupper, round, int, cset, copy, swap, move
    int column2; // for functions copy, swap, move
    char str[CELL_LENGTH]; // for cset function
    row_selection selection;
}data_processing;

//endregion

//region Functions
/**
 * Bubble sorts an array and deletes repeated symbols in array
 * @param victims array to sort and delete replicas
 * @param size size of an array
 */
void sort_del_reps(unsigned int victims[],  unsigned int *size)
{
    unsigned int i, j;
    for (i = 0; i < *(size)-1; i++ )
    {
        for (j = 0; j < *(size)-i-1; j++)
        {
            if (victims[j] > victims[j+1])
            {
                int temp = victims[j];
                victims[j] = victims[j+1];
                victims[j+1] = temp;
            }
        }
    }
    for(i = 0; i < *(size)-1; )
    {
        if(victims[i] == victims[i+1])
        {
            for(j = i+1; j < *(size); j++)
            {
                victims[j] = victims[j+1];
            }
            *size = *size -1;
        }else i++;
    }
}

/**
 *
 * @param arr Array to measure its length
 * @return Integer which is size of an array
 */
int slen(char *arr)
{
    int length = 0;
    while(arr[++length] != '\0');
    return length;
}

/**
 * Compares two strings
 * @param string1 String to compare
 * @param string2 String to compare
 * @return 1 whether two given strings are equal otherwise 0
 */
int scmp(char *string1, char *string2)
{
    int len1;
    if((len1 = slen(string1)) != slen(string2)){return 0;}

    while(len1--)
    {
        if(string1[len1] != string2[len1]){return 0;}
    }
    return 1;
}

/**
 * Prints first size+1 chars of string
 * @param string_to_print
 * @param size
 */
int print(row_info *info)
{
    if(MAX_ROW_LENGTH+1 < info->i)
    {
        return MAX_LENGTH_REACHED;
    }else
    {
        int s = 0;

        while(s <= info->i )
        {
            if(info->cache[s] == EOF)
            {
                return 1;
            }else
            if(info->cache[s] == 0)
            {

                //putchar('|'); // is a dead symbol
                s++;
                continue;
            }else
            if( info->cache[s] == 7)
            {
                //putchar('+'); // 7 is dead separator
                s++;
                continue;
            }else
            if( info->cache[s] == 3 ) // is a baby-separator
            {
                putchar(info->seps.separators[0]);
                s++;
                continue;
            }
            putchar(info->cache[s]);
            s++;
        }
        info->current_row++;
        info->i = 0;
        return 0;
    }

}

/**
 * Checks if char of char is number.
 * @param suspect char to check if it is number
 * @return 1 if true otherwise 0
 */
char isnumber(char suspect)
{
    return (suspect >= 48 && suspect <= 57) ? 1 : 0;
}
//endregion

//region functions for row_info
/**
 * Initialise sepatrators from delim
 * @param argv2 Delim string - string with entered separators
 * @param info information about unique row.
 * @return error_code if delim contains 'bad' symbols otherwise 0
 */
int separators_init( char* argv2, row_info *info )
{
    //region variables
    info->seps.number_of_seps = 0;
    char switcher;
    int j;
    int k = 0;
    //endregion
    //length = slen(argv2);
    //while(k < length)
    while(argv2[k] != '\0')
    {
        /**
         * Prevent user to enter theese symbols for correct functioninig of the program
         */
        if((argv2[k] >= 'a' && argv2[k] <= 'z')
            || (argv2[k] >= 'A' && argv2[k] <= 'Z')
            || (argv2[k] >= -1 && argv2[k] <= 32)
            || (argv2[k] == 45 || argv2[k] == 46)
            || (argv2[k] >= '0' && argv2[k] <= '9'))
        {
            return UNSUPPORTED_SEPARATORS_ERROR;
        }
        switcher = 0;

        j = k+1;
       // while(j < length)
        while(argv2[j] != '\0')
        {
            if(argv2[k] == argv2[j])
            {
                switcher++;
                j++;
                continue;
            }
            j++;
        }
        if(switcher == 0)
        {
            info->seps.separators[info->seps.number_of_seps] = argv2[k];
            info->seps.number_of_seps++;
        }
        k++;
    }

    return 0;
}

/**
 * Check if char from info.cache is separator
 * @param position Position of an element inn info.cache
 * @param info structure with all information about a row
 * @param repl
 *      2 to increase number of separators in a row and replace by the first separator from delim
 *      1 to replace found separator by the first separator in delim
 *      0 to only check if some char from cache is separator
 * @return 1 if separator otherwise 0
 */
int checksep(int position, row_info *info, char repl)
{
    unsigned char j = 0;

    while(j < info->seps.number_of_seps )
    {
        if( info->cache[position] == info->seps.separators[j] )
        {
            if(repl)
            {
                info->cache[position] = info->seps.separators[0];
            }
            if(repl == 2 )
            {
                info->row_seps.number_of_seps++;
            }
            return 1;
        }
        j++;
    }
    return 0;
}

/**
 * Determines number of separators in the row with replaced separators
 * @param row info for access the cache and number_of_separators
 */
void num_of_seps(row_info* info)
{
    int j = 0;

    for(int k = 0; k <= info->i; k++ )
    {
        if(info->cache[k] == info->seps.separators[0] )
        {
            info->row_seps.number_of_seps++;
        }
        j++;
    }
}

/**
 * Initialize information about the row:
 *      positions of last separator of each column
 *      number of columns
 * @param info Structure with information about the row.
 */
void row_info_init(row_info* info )
{
    int j = 0;
    int last_se = 0;
    if(info->cache[j] == -1) return;

    /**
     * go to the end of the column in this cycle
     * checks every char if is a separator
     * adds position of last separator of each column to an array
     */
    while(1)
    {
        if(info->cache[j] == info->seps.separators[0] || info->cache[j] == 7)
        {
            info->last_s[last_se] = j;
            last_se++;
        }else
        if(info->cache[j] == 10)
        {
            info->last_s[last_se] = j;
            break;
        }
        j++;
    }
    // Also num_of_cols ээis len of array with last separators
    info->num_of_cols = ++last_se;
}
//endregion

//region Table edit
/**
 *
 * @param argc Number of argemuments
 * @param argv Array with arguments
 * @param tedit_s main structure for table edit function keywords
 * @param is_dlm if user entered -d : is_dlm is 1 otherwise 0
 * @return 0 if success otherwise error code
 */
int tef_init(int argc, char* argv[], table_edit* tedit_s, char is_dlm)
{
    //region variables
    int position = (is_dlm) ? 3 : 1;
    int from;
    int to;
    tedit_s->d_col.called = 0;
    tedit_s->d_row.called = 0;
    tedit_s->a_col.called = 0;
    tedit_s->a_row.called = 0;
    tedit_s->i_row.called = 0;
    tedit_s->i_col.called = 0;
    //endregion

    while(position < argc)
    {
        if(scmp(argv[position], "dcol"))
        {
            to = atoi(argv[position + 1]);
            if(position + 1 < argc && to > 0)
            {
                tedit_s->d_col.victims[tedit_s->d_col.called++] = to;
                position++;
                continue;
            }else return BAD_ARGUMENTS_ERROR_DCOL;
        }else
        if(scmp(argv[position], "dcols"))
        {
            from = atoi(argv[position+1]);
            to = atoi(argv[position+2]);
            if(position+2 < argc && from > 0 && to >= from)
            {
                for(int r = from; r <= to; r++)
                {
                    tedit_s->d_col.victims[tedit_s->d_col.called++] = r;
                }
                position+=3;
                continue;
            } else return BAD_ARGUMENTS_ERROR_DCOLS;
        }else
        if(scmp(argv[position], "drow"))
        {
            to = atoi(argv[position+1]);
            if(position+1 < argc && to > 0)
            {
                tedit_s->d_row.victims[tedit_s->d_row.called++] = to;
                position+=2;
                continue;
            }else return BAD_ARGUMENTS_ERROR_DROW;
        }else
        if(scmp(argv[position], "drows"))
        {
            from = atoi(argv[position+1]);
            to = atoi(argv[position+2]);
            if(position+2 < argc && from > 0 && to >= from)
            {
                for(int r = from; r <= to; r++)
                {
                    tedit_s->d_row.victims[tedit_s->d_row.called++] = r;
                }
                position+=3;
                continue;
            }else return BAD_ARGUMENTS_ERROR_DROWS;
        }else
        if(scmp(argv[position], "acol"))
        {
            tedit_s->a_col.called++;
        }else
        if(scmp(argv[position], "icol"))
        {
            to = atoi(argv[position+1]);
            if(position+1 < argc && to > 0)
            {
                tedit_s->i_col.victims[tedit_s->i_col.called++] = to;
                position+=2;
                continue;
            }else return BAD_ARGUMENTS_ERROR_ICOL;
        }
        if(scmp(argv[position], "arow"))
        {
            tedit_s->a_row.called++;
        }else
        if(scmp(argv[position], "irow"))
        {
            to = atoi(argv[position+1]);
            if(position+1 < argc && to > 0)
            {
                tedit_s->i_row.victims[tedit_s->i_row.called++] = to;
                position+=2;
                continue;
            }else return BAD_ARGUMENTS_ERROR_IROW;
        }
        position++;
    }

    if(tedit_s->d_col.called > 1)
    {
        sort_del_reps(tedit_s->d_col.victims, &(tedit_s->d_col.called));
    }
    if(tedit_s->i_col.called > 1)
    {
        sort_del_reps(tedit_s->i_col.victims, &(tedit_s->i_col.called));
    }
    if(tedit_s->d_row.called > 1)
    {
        sort_del_reps(tedit_s->d_row.victims, &(tedit_s->d_row.called));
    }
    if(tedit_s->i_row.called > 1)
    {
        sort_del_reps(tedit_s->i_row.victims, &(tedit_s->i_row.called));
    }

    return 0;
}

/**
 * Inserts an empty column after the last column in each row
 * @return 0 if col has been added otherwise error code
 */
int acol_f(row_info* info)
{
    if(info->cache[info->i] == EOF )
    {
       return 0;
    }
    if( info->i+1 <= MAX_ROW_LENGTH )
    {
        info->cache[info->i] = info->seps.separators[0];
        info->cache[(++info->i)] = 10;
        info->row_seps.number_of_seps++;
    } else return MAX_LENGTH_REACHED;
    return 0;
}

/**
 * Inserts a new row after the last line
 */
void arow_f(row_info* info)
{
    if(info->cache[info->i] == EOF && MAX_ROW_LENGTH > info->i)
    {
        char temp = info->cache[info->i];
        for(int k = 0; k < info->row_seps.number_of_seps; k++)
        {
            info->cache[info->i++] = info->seps.separators[0];
        }
        info->cache[info->i] = 10;
        info->cache[++(info->i)] = temp;
    }
}

/**
 * Removes the column number R > 0
 * @param victim_column Column to delete
 * @param info All information about the row
 * @return 0 if success otherwise error code
 */
int dcol_f(int victim_column, row_info* info)
{
    if(info->cache[info->i] == EOF){return 0;}

    if(victim_column > info->num_of_cols)
    {
        return COLUMN_OUT_OF_RANGE_ERROR;
    }else
    {
        int j = info->last_s[victim_column-1];

        if(info->cache[j] != 10 && info->cache[j] != EOF){ info->cache[j] = 7; }
        j--;

        while( j >= 0 && info->cache[j] != info->seps.separators[0] )
        {
            //if(j == 0 && (checksep(j, info, 0) || info->cache[0] == 7 ))
            if(j == 0 && info->cache[0] == 7 )
            {
                info->cache[0] = 7;
                break;
            }else
            if(j!= 0 && info->cache[j] == 7)
            {
                j--;
                continue;
            }else
            {
                info->cache[j] = 0;
                j--;
            }
        }

        if((info->cache[info->last_s[victim_column-1]] == 10 || info->cache[info->last_s[victim_column-1]] == EOF))
        {
            info->cache[j] = 0;
        }
        info->row_seps.number_of_seps--;
        return 0;
    }
}

/**
 * Removes the row number R > 0
 * @param victim_row Row to remove
 * @param info Information about the row
 * @return
 */
void drow_f(int victim_row, row_info* info)
{
    if(info->current_row == victim_row)
    {
        for(int j = 0; j <= info->i; j++)
        {
            if(info->cache[j] == info->seps.separators[0] || info->cache[j] == 7)
            {
                info->cache[j] = 7;
            } else info->cache[j] = 0;
        }
    }
}

/**
 * Inserts the column before the column R > 0. Inserts char 3 and in print() changes it to separator
 * @param victim_column insert column before this nuber
 * @param info Information about a row and separators
 * @return 0 if success otherwise error code
 */
int icol_f(int victim_column, row_info* info)
{
    if(info->i+1 > MAX_ROW_LENGTH)
    {
        return MAX_LENGTH_REACHED;
    }
    if(info->cache[info->i] == EOF)
    {
        return 0;
    }
    int j;
    int stop = 0;
    if(victim_column != 1)
    {
        stop = info->last_s[victim_column-2];
    }
    for( j = info->i+1; j > stop; j--)
    {
        info->cache[j] = info->cache[j-1];
    }

    /** 3 is a "baby separator", who is not separators yet and
     *  becomes a separator only in print funnction
     *  Program prevents problems with compatibility dcol and icol functions using this
     */
    info->cache[j] = 3;

    info->row_seps.number_of_seps++;
    info->i++;
    return 0;
}

/**
 * Inserts the row before the row R > 0
 * @param victim_row insert row before this nuber
 * @param info all information about the row
 * @return
 */
void irow_f(int victim_row, row_info* info)
{
    if(info->current_row == victim_row)
    {
        for(int j = 0; j < info->row_seps.number_of_seps; j++)
        {
            putchar(info->seps.separators[0]);
        }
        putchar(10);
    }
}
//endregion

//region Data processing
int dpf_init(int argc, char* argv[], data_processing* daproc, char is_dlm)
{

    //region variables
    int position = (is_dlm) ? 3 : 1;
    int first_arg = position;
    int from;
    int to;

    /**
     * By default it means processing only last row
     */
    daproc->selection.from = 0;
    daproc->selection.to = 0;

    /**
     * Dpf_option is data processing function option
     * Each function has its own option.
     * 0 means there is not dpf functino in arguments
     */
    daproc->dpf_option = NO_DPF;
    /**
     * The same thing with row selection
     * 0 by default, but if there is row selection in arguments it becomes a number, defines the certain selection
     */
    daproc->selection.rs_option = NO_SELECTION;
    //endregion

    while(position < argc)
    {
        if(position == first_arg)
        {
            if(scmp(argv[position], "rows"))
            {
                if(position+3 < argc)
                {
                    from = atoi(argv[position+1]);
                    to = atoi(argv[position+2]);

                    if(from == 0 && argv[position+1][0] == '-' && argv[position+1][1] == '\0')
                    {
                        from = 0;
                    } else return SELECTION_IOOF_ERROR;

                    if(to == 0 && argv[position+2][0] == '-' && argv[position+2][1] == '\0')
                    {
                        to = 0;
                    } else return SELECTION_IOOF_ERROR;


                    daproc->selection.rs_option = ROWS;
                    daproc->selection.from = from;
                    daproc->selection.to = to;
                    position++;
                    continue;
                }else return TOO_FEW_ARGS_AFTER_SELECTION;
            }else
            if(scmp(argv[position], "beginswith"))
            {
                if(position+3 < argc)
                {
                    from = atoi(argv[position+1]);

                    if(from <= 0)
                    {
                        return SELECTION_IOOF_ERROR;
                    }

                    daproc->selection.rs_option = BEGINSWITH;
                    daproc->selection.from = from;
                    strcpy(argv[position+2], daproc->selection.pattern);
                    position++;
                    continue;
                }else return TOO_FEW_ARGS_AFTER_SELECTION;
            }else
            if(scmp(argv[position], "contains"))
            {
                if(position+3 < argc)
                {
                    from = atoi(argv[position+1]);
                    if(from <= 0)return SELECTION_IOOF_ERROR;
                    daproc->selection.rs_option = CONTAINS;
                    daproc->selection.from = from;
                    strcpy(argv[position+2], daproc->selection.pattern);
                    position++;
                    continue;
                }else return TOO_FEW_ARGS_AFTER_SELECTION;
            }
        }

        if(position+1 < argc)
        {
            if((daproc->column1 = atoi(argv[position+1])) > 0)
            {
                if(scmp(argv[position], "tolower"))
                {
                    daproc->dpf_option = TOLOWER;
                    break;
                }else
                if(scmp(argv[position], "toupper"))
                {
                    daproc->dpf_option = TOUPPER;
                    break;
                }else
                if(scmp(argv[position], "round"))
                {
                    daproc->dpf_option = ROUND;
                    break;
                }else
                if(scmp(argv[position], "int"))
                {
                    daproc->dpf_option = INT;
                    break;
                }
            }else return DPF_IOOF_ERROR;
        } else return TOO_FEW_ARGS_AFTER_DPF;

        if(scmp(argv[position], "cset"))
        {
            if(position+2 < argc)
            {
                from = atoi(argv[position+1]);
                if(from <= 0)return SELECTION_IOOF_ERROR;
                daproc->column1 = from;
                strcpy(daproc->str, argv[position+2]);
                daproc->dpf_option = CSET;
                break;
            }else return TOO_FEW_ARGS_AFTER_DPF;
        }else
        if(scmp(argv[position], "copy"))
        {
            if(argc > position+2)
            {
                daproc->dpf_option = COPY;
                break;
            }else return TOO_FEW_ARGS_AFTER_DPF;
        }else
        if(scmp(argv[position], "swap"))
        {
            if(argc > position+2)
            {
                daproc->dpf_option = SWAP;
                break;
            }else return TOO_FEW_ARGS_AFTER_DPF;
        }else
        if(scmp(argv[position], "move"))
        {
            if(argc > position+2)
            {
                daproc->dpf_option = MOVE;
                break;
            }else return TOO_FEW_ARGS_AFTER_DPF;
        }
        position++;
    }

    return 0;
}

void tolower_f(row_info *info, data_processing *daproc)
{
    int from = (daproc->column1 == 1) ? 0 : info->last_s[daproc->column1-2];

    for( ; from < info->last_s[daproc->column1-1]; from++)
    {
        if(info->cache[from] >= 'A' && info->cache[from] <= 'Z') info->cache[from] += 32;
    }
}

void toupper_f(row_info *info, data_processing *daproc)
{
    int from = (daproc->column1 == 1) ? 0 : info->last_s[daproc->column1-2];

    for( ; from < info->last_s[daproc->column1-1]; from++)
    {
        if(info->cache[from] >= 'a' && info->cache[from] <= 'z') info->cache[from] -= 32;
    }

}

void round_f(row_info *info, data_processing *daproc)
{
    int from = (daproc->column1 == 1) ? 0 : info->last_s[daproc->column1-2]+1;
    int to = info->last_s[daproc->column1-1];
    int temp = from;

    for( ; from < to; from++ )
    {
        if(info->cache[from] == '.')
        {
            if(info->cache[from+1] >= '5' && info->cache[from+1] <= '9')
            {

                int j;
                for( j = from; j > temp; j--)
                {
                    info->cache[j] = info->cache[j-1];
                }
                info->cache[j] = '0';

                j = from+1;
                while(j < to)
                {
                    info->cache[j] = 0;
                    j++;
                }

                while(info->cache[from] == '9')
                {
                    info->cache[from] = '0';
                    from--;
                }
                info->cache[from]++;
                if(info->cache[temp] == '0') info->cache[temp] = 0;


            }else
            if(info->cache[from+1] >= 0 && info->cache[from+1] < '5')
            {
                while(from < to)
                {
                    info->cache[from] = 0;
                    from++;
                }
                return;
            }
        }
    }
}

void int_f(row_info *info, data_processing *daproc)
{

    int from = 0;
    int to = info->last_s[daproc->column1-1];

    if(daproc->column1 != 1) from = info->last_s[daproc->column1-2]+1;

    while(from < to)
    {
        if(info->cache[from] == '.')
        {
            info->cache[from] = 0;
            from++;
            while(from < to)
            {
                info->cache[from] = 0;
                from++;
            }
            return;
        }
        from++;
    }

}

void copy_f(row_info *info, data_processing *daproc)
{
    (void)info;
    (void)daproc;
}

void move_f(row_info *info, data_processing *daproc)
{
    (void)info;
    (void)daproc;
}

void swap_f(row_info *info, data_processing *daproc)
{
    (void)info;
    (void)daproc;
}

void cset_fun(row_info *info, data_processing *daproc)
{
    (void)info;
    (void)daproc;
}
//endregion

void dpf_call(row_info *info, data_processing *daproc )
{

    void (*function_to_call)(row_info *info ,data_processing *daproc);
    switch(daproc->dpf_option)
    {
        case TOLOWER:
            function_to_call = &tolower_f;
            break;
        case TOUPPER:
            function_to_call = &toupper_f;
            break;
        case ROUND:
            function_to_call = &round_f;
            break;
        case INT:
            function_to_call = &int_f;
            break;
        case COPY:
            function_to_call = &copy_f;
            break;
        case MOVE:
            function_to_call = &move_f;
            break;
        case SWAP:
            function_to_call = &swap_f;
            break;
        case CSET:
            function_to_call = &cset_fun;
            break;
        case NO_DPF:
            return;
    }
    if(info->cache[0] == -1) return;
    if(daproc->dpf_option == ROUND || daproc->dpf_option == INT)
    {
        char negative = 0;
        char point = 0;
        int j = 0;

        if(daproc->column1 != 1) j = info->last_s[daproc->column1-2]+1;

        for( ; j < info->last_s[daproc->column1-1]; j++)
        {
            if(!isnumber(info->cache[j]))
            {
                if(isnumber(info->cache[j])) continue;
                if(info->cache[j] == '-' && !negative)
                {
                    negative++;
                }else
                if(info->cache[j] == '.' && !point)
                {
                    point++;
                }else return;
            }
        }

    }
    switch(daproc->selection.rs_option)
    {
        case ROWS:
            if(info->current_row >= daproc->selection.from && info->current_row <= daproc->selection.to)
            {
                function_to_call(&(*info), &(*daproc));
            }
            break;
        case BEGINSWITH:
            printf("beginswith");
//            function_to_call(&(*info), &(*daproc));
            break;
        case CONTAINS:
            printf("contains");
            function_to_call(&(*info), &(*daproc));
            break;
        case NO_SELECTION:
            function_to_call(&(*info), &(*daproc));
            return;
    }


}
/**
 *
 * @param argc Number of arguments of command line
 * @param argv Values arguments of command line
 * @return 0 if success else error
 */
int cache_init(int argc, char* argv[])
{
    if(argc >= 100) return TOO_MUCH_ARGUMENTS_ERROR;
    //region variables
    row_info info;
    info.i = 0;
    info.current_row = 1;

    int seps_first_line = 0;
    char is_dlm = 0;
    int j; // a variable to pass through all command line arguments

    table_edit tedit;
    data_processing daproc;

    int seps_diff = 0;

    /**
     * By default it is 0 but if there is different number of separators in different lines
     *  it becomes an error code. Program will be not terminated, but user will be warned
     *  that program cannot work properly
     */
    int error_wrong_number_of_seps = 0;

    /**
     * By defult it is 0, but functons can change this value. In this case program will be terminated
     */
    int exit_code = 0;
    //endregion


    if(argc > 2 && !scmp(argv[1], "-d"))
    {
        info.seps.number_of_seps = 1;
        info.seps.separators[0] = ' ';
    }else
    if(argc > 3 && scmp(argv[1], "-d"))
    {
        is_dlm = 1;
        exit_code = separators_init(argv[2], &info);
        if(exit_code)
        {
            return exit_code;
        }
    }
    else return TOO_FEW_ARGS_ERROR;


    if((exit_code = tef_init(argc, argv, &tedit, is_dlm))) return exit_code;


    if((exit_code = dpf_init(argc, argv, &daproc, is_dlm))) return exit_code;

    /**
     * Scan the file line by line
     * if we find \n char go to the function parsing, parse functons, give line to the functions
     *
     */
    do
    {
        info.cache[info.i] = getchar();
        checksep(info.i, &info, 2);

        if(info.i == MAX_ROW_LENGTH-1 && info.cache[info.i] != 10)
        {
            return MAX_LENGTH_REACHED;
        }

        if(info.cache[info.i] == 10 || info.cache[info.i] == EOF)
        {

            row_info_init(&info);

            if(info.cache[info.i] != EOF)
            {
                info.row_seps.number_of_seps = 0;
                num_of_seps(&info);

                if(info.current_row == 1)
                {
                    seps_first_line = info.row_seps.number_of_seps;
                }
            }
            seps_diff = seps_first_line - info.row_seps.number_of_seps;
            if(seps_diff != 0 )
            {
                error_wrong_number_of_seps = WRONG_NUMBER_OF_SEPS_ERROR;
            }


           dpf_call(&info, &daproc);


            //row_info_init(&info); // to make possible dpf with tef


            //region table edit functions
            for(j = 0; j < (int)tedit.d_col.called; j++)
            {
                if((exit_code = dcol_f(tedit.d_col.victims[j], &info))){ return exit_code; }
            }
            for(j = 0; j < (int)tedit.a_col.called; j++)
            {
                if((exit_code = acol_f(&info))){ return exit_code; }
            }
            for(j = tedit.i_col.called-1; j >= 0 ; j--)
            {
                if((exit_code = icol_f(tedit.i_col.victims[j], &info))){ return exit_code; }
            }
            for(j = 0; j < (int)tedit.a_row.called; j++)
            {
                arow_f(&info);
            }
            for(j = 0; j < (int)tedit.d_row.called; j++)
            {
                drow_f(tedit.d_row.victims[j], &info);
            }
            for(j = 0; j < (int)tedit.i_row.called; j++)
            {
                irow_f(tedit.i_row.victims[j], &info);
            }
            //endregion



            if((exit_code = print(&info)))
            {
                if(exit_code == 1)
                {
                    break;
                }
                return exit_code;
            }
            continue;
        }
        (info.i)++;
    }while(1);
    return error_wrong_number_of_seps;
}

int main(int argc, char* argv[])
{
    int exit_code = 0;
    char* error_msg[] = {
            "ERROR: Empty stdin. There is no in input to edit it\n", //0
            "ERROR: One of the rows is too long. Try to make in shorter\n", //1
            "ERROR: Separators you entered are not supported by the program to prevent undetermined behaviour"
            ". They mustnt be numbers, letters or symbols from 0 to 10 ASCII values \n", //2
            "WARNING: There is different number of separators in differrent lines,"
            " program may not work properly. Check each line for number of separators or make sure "
            "you entered all possible separators in DELIM \n", //3
            "ERROR: Different number of separators in different rows. Correct the input file\n", //4
            "ERROR: The program supports at most one row selector. Enter less selectors\n", //5
            "ERROR: Too much arguments. Program supports at most 100 arguments. Enter less arguments\n", //6
            "ERROR: Wrong parameters after dcol. Enter parameter greater than 0 after dcol\n",//7
            "ERROR: Wrong parameters after dcols. 1-st parameter must be less than 2-nd. Both must be greater than 0\n",//8
            "ERROR: Wrong parameters after drow. Enter parameter greater than 0 after drow\n",//9
            "ERROR: Wrong parameters after drows. 1-st parameter must be less or equal than 2-nd. Both must be greater than 0\n",//10
            "ERROR: Wrong parameters after irow. Enter parameter greater than 0 after irow\n",//11
            "ERROR: Wrong parameters after icol. Enter parameter greater than 0 after icol\n",//12
            "ERROR: Column you entered is out of range\n",//13
            "ERROR: Too few arguments after row selection command. Enter more arguments\n", //14
            "ERROR: Too few arguments after data processing command\n", //15 - returns 16
            "ERROR: Index after data processing command is out of range\n",
            "ERROR: Too no functions or delim string in command line . "
            


            //"Wrong separators in delim. Some dymbols from delim(argument after -d) cannot be used as separators\n" //106
            };

    exit_code = cache_init(argc, argv);
    if(exit_code > 1)
    {
        fprintf(stderr, "%s", error_msg[exit_code-2] );
    }

    return exit_code;
}
