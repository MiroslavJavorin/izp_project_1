//region Includes
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//endregion

//region defines
//region Errors
enum errors
{
    EMPTY_STDIN_ERROR = 2,
    MAX_LENGTH_REACHED,
    UNSUPPORTED_SEPARATORS_ERROR,
    WRONG_SEPARATORS_ERROR,
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
    WRONG_NUMBER_OF_ARGS, // selection <= 0 or is a letter
    DPF_IOOF_ERROR,
    SELECTION_IOOF_ERROR, //20
    TOO_FEW_ARGS_ERROR,
    SELECTION_TOO_LONG_PATTERN_ERROR,
    NON_INT_ARGUMENTS_ERROR,
    MAX_CELL_LENGHT_ERROR,
    NO_ARGUMENTS_ERROR,
    EXCESS_ARGUMENTS_ERROR
};
//endregion

//region Lengths
#define MAX_ROW_LENGTH          10240
#define CELL_LENGTH              100
#define SEPS_SIZE                127
#define MAX_NUMBER_OF_ARGUMENTS  101
//endregion

//region dpf_option
enum dpf_functions_enum
{
    NO_DPF,
    TOLOWER,TOUPPER,ROUND, INT, // 1 number as parameter
    COPY,MOVE,SWAP, // 2 numbers as parameters
    CSET, // Number and string as parameters
    CSUM,CAVG,CMIN,CMAX,CCOUNT,
    CSEQ, // 3 numbers as parameters
    RSEQ, // 4 numbers as parameters
    RSUM,RAVG,RMIN,RMAX,RCOUNT
};
enum dpf_rows_selection_enum
{
    NO_SELECTION,
    ROWS,
    BEGINSWITH,CONTAINS
};
//endregion

//region Structures
/**
 * Contains information about separators from delim string
 */
typedef struct
{
    char separators[SEPS_SIZE];
    int number_of_seps;
}seps_struct;

/**
 * Structure for arithmetical functions as
 *  CSUM, CAVG, CMIN, CMAX, CCOUNT, CSEQ, RSUM, RAVG, RMIN, RMAX, RCOUNT,
 */
typedef struct
{
    float sum;
    unsigned long long int valid_row; // to check if row can be used for arithmetical functions.
    float min_max;

    //if there is a very big table with lots of rows the date type int may not be enough
    unsigned long long int empties;
}arithm_t;

/**
 * Contains informatoin about scanned row
 */
typedef struct
{
    int i; // position of the last element of the row
    int current_row;
    char cache[MAX_ROW_LENGTH]; // cache contains one single row of the file
    seps_struct seps;
    seps_struct row_seps;
    int last_s[MAX_ROW_LENGTH]; // array woth positions of right separators of cells
    int num_of_cols;
    char is_lastrow;
    arithm_t arithmetic;
}row_info;

typedef struct
{
    // victims are arguments after function name
    unsigned int victims[MAX_ROW_LENGTH];
    unsigned int called; // number of calling of single function to iterate through an array of "victims"
}tef_t;

typedef struct
{
    char arg_count;
    tef_t d_col;
    tef_t i_col;
    tef_t a_col;
    tef_t a_row;
    tef_t i_row;
    tef_t d_row;
}table_edit;

typedef struct
{
    /**
     * rs_option says which data processing function program will use
     */
    char rs_option;
    int from;
    int to;
    int pattern_len;
    char pattern[CELL_LENGTH];
}row_selection;

typedef struct
{
    char arg_count;
    char dpf_option; //cset tolower toupper...  1 2 3 4 ..
    int number1; // for functions cset, tolower, toupper, round, int, cset, copy, swap, move
    int number2; // for functions copy, swap, move
    int number3; // for functions csum, cavg, cmax, ccount, cseq, rsum, ravg, rmin, rmax, rcount
    float number4; // for rseq function
    char str[CELL_LENGTH]; // for cset function
    row_selection selection;
}data_processing;

//endregion

//region Functions
/**
 * Bubble sorts an array and deletes repeated symbols in it
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


int slen(char *arr)
{
    int length = 0;
    while(arr[++length] != '\0');
    return length;
}

/**
 * Checks if the string contains the pattern
 *
 * @param pattern
 * @param string
 * @return 1 if the string contains the pattern
 */
int find(char *pattern, char *string)
{
    for (int i=0; string[i]; ++i)
    {
        for (int j=0; ; ++j)
        {
            if (!pattern[j])
                return 1;
            if(string[i+j]!=pattern[j])
                break;
        }
    }
    return 0;
}

/**
 * Compares two strings
 * @param string1
 * @param string2
 * @return 1 whether two given strings are equal otherwise 0
 */
int scmp(char *string1, char *string2)
{
    int len1;
    if((len1 = slen(string1)) != slen(string2)) return 0;

    while(len1--)
    {
        if(string1[len1] != string2[len1])
            return 0;
    }
    return 1;
}

/**
 * Prints cache to the stdout.
 * If there are characters in the cache that should not be written out
 *  or should be replaced with other characters, it does so.
 *  This is done so that some functions do not conflict with each other.
 * @param string_to_print
 * @param size
 */
int print(row_info *info)
{
    if(MAX_ROW_LENGTH+1 < info->i)
    {
        return MAX_LENGTH_REACHED;
    }
    else
    {
        int s = 0;
        while(s <= info->i )
        {
            if(info->cache[s] == EOF)
            {
                return 1;
            }else if(info->cache[s] == 0)
                {
                    s++;
                    continue;
                }else if( info->cache[s] == 7)
                    {
                        s++;
                        continue;
                    }else if( info->cache[s] == 3 ) // is a baby-separator
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
 * @return returns an error code if the string contains characters that could cause undefined program behavior
 *  otherwise 0
 */
int separators_init(char* argv2, row_info *info)
{
    //region variables
    info->seps.number_of_seps = 0;
    char switcher;
    int j;
    int k = 0;
    //endregion

    while(argv2[k] != '\0')
    {
        /**
         * Prevent user to enter theese symbols for correct functioninig of the program
         */
        if((argv2[k] >= 'a' && argv2[k] <= 'z') ||
           (argv2[k] >= 'A' && argv2[k] <= 'Z') ||
           (argv2[k] >= -1 && argv2[k] < 32) ||
           (argv2[k] == 45 || argv2[k] == 46)
                )
        {
            return UNSUPPORTED_SEPARATORS_ERROR;
        }
        switcher = 0;

        j = k+1;
        while(argv2[j])
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
 * Initializes information about the row:
 *   positions of last separator of each column
 *   number of columns
 *
 *  Checks if the lenght of cell is less than 100. Otherwise returns error
 *
 *
 * @param info Structure with information about the row.
 * @param option
 *      1:  Check if the given character is a separator
 *      and replace it with the first separator entered by the user.
 *      Then nitialize positions of separators and number of separators
 *      0: Dont do 1 and just initialize number of separators and positions of separators
 * @return error code or 0
 */
int row_info_init(row_info* info, char option)
{
    if(info->cache[0] == EOF) return 0;
    int last_se = 0;
    unsigned char k = 0;

    /**
     * go to the end of the column in this cycle
     * checks every char if is a separator
     * adds position of last separator of each column to an array
     */
    for(int j = 0; j <= info->i; j++)
    {
        if(option)
        {
            while(k < info->seps.number_of_seps)
            {
                if(info->cache[j] == info->seps.separators[k])
                {
                    info->cache[j] = info->seps.separators[0];
                    info->row_seps.number_of_seps++;
                }
                k++;
            }
            k = 0;
        }

        if(info->cache[j] == info->seps.separators[0] || info->cache[j] == 7)
        {
            if(last_se >= 1)
            {
                if(j - info->last_s[last_se-1]-1 > 100)
                    return MAX_CELL_LENGHT_ERROR;
            }else if(j - 0 > 100)
                    return MAX_CELL_LENGHT_ERROR;

            info->last_s[last_se] = j;
            last_se++;
        }else if(info->cache[j] == 10)
            {
                if(last_se >= 1)
                {
                    if(j - info->last_s[last_se-1]-1 > 100)
                        return MAX_CELL_LENGHT_ERROR;
                }else if(j - 0 > 100)
                        return MAX_CELL_LENGHT_ERROR;

                info->last_s[last_se] = j;
                break;
            }else if(option == 1 && info->cache[j] == 3)
                {
                    if(last_se >= 1)
                    {
                        if(j - info->last_s[last_se-1]-1 > 100)
                            return MAX_CELL_LENGHT_ERROR;
                    }else if(j - 0 > 100)
                            return MAX_CELL_LENGHT_ERROR;

                    info->last_s[last_se] = j;
                    last_se++;
                }
    }

    // Also num_of_cols is len of array with last separators
    info->num_of_cols = ++last_se;
    return 0;
}
//endregion

//region Table edit
/**
 *  Parses command line arguments. Determines which functions with which arguments the user has called
 *  Initializes the table_edit structure.
 *
 *  Checks keywords against the arguments entered by the user on the command line
 *
 *  Initializes structures that will later be used to call functions for editing the table.

 *  Counts the number of arguments that refer to functions, then check against the number of arguments
 *  entered on the command line and, if they are not equal, return an error.

 *  Adds arguments of table edit functions into arrays, so that later these functions can be simply called with arguments.
 *  Also sorts arrays and removes duplicate elements so as not to call the same function multiple times
 *
 *
 * @param is_dlm
 * 	  If it is equal to 1, it means that a line with separators has been entered and the parsing of arguments starts from the 3rd argument.
 *    Otherwise, the parsing of arguments starts from the 1st argument.
 * @return error code or 0 if success
 */
int tef_init(int argc, char* argv[], table_edit* tedit_t, char is_dlm)
{
    //region variables
    int position = (is_dlm) ? 3 : 1;
    int from;
    int to;
    tedit_t->d_col.called = 0;
    tedit_t->d_row.called = 0;
    tedit_t->a_col.called = 0;
    tedit_t->a_row.called = 0;
    tedit_t->i_row.called = 0;
    tedit_t->i_col.called = 0;
    tedit_t->arg_count = 0;
    //endregion

    while(position < argc)
    {
        if(scmp(argv[position], "dcol"))
        {
            if(position+1 < argc)
            {
                if((to = atoi(argv[position+1])) - atof(argv[position+1]) || to <= 0)
                    return BAD_ARGUMENTS_ERROR_DCOL;
                tedit_t->d_col.victims[tedit_t->d_col.called++] = to;

                position+=2;
                tedit_t->arg_count+=2;
                continue;
            }else return BAD_ARGUMENTS_ERROR_DCOL;
        }else if(scmp(argv[position], "dcols"))
            {
                if(position+2 < argc)
                {
                    if((from = atoi(argv[position+1])) - atof(argv[position+1]) ||
                       (to = atoi(argv[position+2])) - atof(argv[position+2]) ||
                       to < from ||
                       to <= 0 ||
                       from <= 0)
                    {
                        return BAD_ARGUMENTS_ERROR_DCOLS;
                    }
                    for(int r = from; r <= to; r++)
                        tedit_t->d_col.victims[tedit_t->d_col.called++] = r;

                    tedit_t->arg_count+=3;
                    position+=3;
                    continue;
                } else return BAD_ARGUMENTS_ERROR_DCOLS;
            }else if(scmp(argv[position], "drow"))
                {
                    if(position+1 < argc)
                    {
                        if((to = atoi(argv[position+1])) - atof(argv[position+1]) || to <= 0)
                            return BAD_ARGUMENTS_ERROR_DROW;

                        tedit_t->d_row.victims[tedit_t->d_row.called++] = to;
                        tedit_t->arg_count+=2;
                        position+=2;
                        continue;
                    }else return BAD_ARGUMENTS_ERROR_DROW;
                }else if(scmp(argv[position], "drows"))
                    {
                        if(position+2 < argc )
                        {
                            if((from = atoi(argv[position+1])) - atof(argv[position+1]) ||
                               (to = atoi(argv[position+2])) - atof(argv[position+2]) ||
                               to < from ||
                               to <= 0 ||
                               from <= 0)
                            {
                                return BAD_ARGUMENTS_ERROR_DROWS;
                            }

                            for(int r = from; r <= to; r++)
                                tedit_t->d_row.victims[tedit_t->d_row.called++] = r;

                            tedit_t->arg_count+=3;
                            position+=3;
                            continue;
                        }else return BAD_ARGUMENTS_ERROR_DROWS;
                    }else if(scmp(argv[position], "acol"))
                        {
                            tedit_t->arg_count++;
                            tedit_t->a_col.called++;
                        }else if(scmp(argv[position], "icol"))
                            {
                                if(position+1 < argc)
                                {
                                    if((to = atoi(argv[position+1])) - atof(argv[position+1]) || to <= 0)
                                        return BAD_ARGUMENTS_ERROR_ICOL;

                                    tedit_t->i_col.victims[tedit_t->i_col.called++] = to;

                                    tedit_t->arg_count+=2;
                                    position+=2;
                                    continue;
                                }else return BAD_ARGUMENTS_ERROR_ICOL;
                            }else if(scmp(argv[position], "arow"))
                                {
                                    tedit_t->arg_count++;
                                    tedit_t->a_row.called++;
                                }else if(scmp(argv[position], "irow"))
                                    {
                                        if(position+1 < argc)
                                        {
                                            if((to = atoi(argv[position+1])) - atof(argv[position+1]) || to <= 0)
                                                return BAD_ARGUMENTS_ERROR_IROW;

                                            tedit_t->i_row.victims[tedit_t->i_row.called++] = to;
                                            tedit_t->arg_count+=2;
                                            position+=2;
                                            continue;
                                        }else return BAD_ARGUMENTS_ERROR_IROW;
                                    }
        position++;
    }

    if(tedit_t->d_col.called > 1) sort_del_reps(tedit_t->d_col.victims, &(tedit_t->d_col.called));
    if(tedit_t->i_col.called > 1) sort_del_reps(tedit_t->i_col.victims, &(tedit_t->i_col.called));
    if(tedit_t->d_row.called > 1) sort_del_reps(tedit_t->d_row.victims, &(tedit_t->d_row.called));
    if(tedit_t->i_row.called > 1) sort_del_reps(tedit_t->i_row.victims, &(tedit_t->i_row.called));

    return 0;
}

/**
 * Inserts an empty column after the last column in each row
 *
 * @return 0 if col has been added otherwise error code
 */
int acol_f(row_info* info)
{
    if(info->cache[info->i] == EOF)
        return 0;
    if(info->i+1 <= MAX_ROW_LENGTH)
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
            info->cache[info->i++] = info->seps.separators[0];

        info->cache[info->i] = 10;
        info->cache[++(info->i)] = temp;
    }
}

/**
 * Removes the column number R > 0
 *
 * @return 0 if success otherwise error code
 */
int dcol_f(int victim_column, row_info* info)
{
    if(info->cache[info->i] == EOF)
        return 0;
    if(victim_column > info->num_of_cols)
        return COLUMN_OUT_OF_RANGE_ERROR;

    int from = (victim_column == 1) ? 0 : info->last_s[victim_column-2];
    int to = info->last_s[victim_column-1];
    if(info->cache[to] == 10) to--;
    char flip_sep = 1;

    if(info->cache[to] == 10)
    {
        to--;
        printf("from=%d, to=%d\n", from , to);
    }


    while(to >= from)
    {
        if(info->cache[to] == info->seps.separators[0] && flip_sep )
        {
            flip_sep = 0;
            info->cache[to] = 7;
            to--;
            continue;
        } else if(info->cache[to] == info->seps.separators[0] && !flip_sep)
            {
                info->cache[to] = info->seps.separators[0];
                flip_sep = 1;
                to--;
                continue;
            }
        if(info->cache[to] == 7)
        {
            to--;
            continue;
        }
        info->cache[to--] = 0;
    }


    if(--info->row_seps.number_of_seps == -1)
        info->row_seps.number_of_seps++;
    return 0;
}

/**
 * Removes the row number R > 0
 */
void drow_f(int victim_row, row_info* info)
{
    if(info->current_row == victim_row-1)
    {
        for(int j = 0; j <= info->i; j++)
        {
            if(info->cache[j] == info->seps.separators[0] || info->cache[j] == 7)
                info->cache[j] = 7;
            else
                info->cache[j] = 0;
        }
    }
}

/**
 * Inserts the column before the column R > 0. Inserts char 3 and in print() changes it to separator
 *
 * @return 0 if success otherwise error code
 */
int icol_f(int victim_column, row_info* info)
{
    if(info->i+1 > MAX_ROW_LENGTH)
        return MAX_LENGTH_REACHED;
    if(info->cache[info->i] == EOF)
        return 0;

    int j;
    int stop = (victim_column == 1) ? 0 : info->last_s[victim_column-2];

    for(j = info->i+1; j > stop; j--)
        info->cache[j] = info->cache[j-1];

    /** 3 is a "baby separator", who is not separators yet and
     *  becomes a separator only in print fucnction
     *  Program prevents problems with compatibility dcol and icol functions using this
     */
    info->cache[j] = 3;
    info->row_seps.number_of_seps++;
    info->i++;
    return 0;
}

/**
 * Inserts the row before the row R > 0
 */
void irow_f(int victim_row, row_info* info)
{
    if(info->current_row == victim_row-1)
    {
        for(int j = 0; j < info->row_seps.number_of_seps; j++)
            putchar(info->seps.separators[0]);
        putchar(10);
    }
}
//endregion

//region Data processing
/**
 *  Parses command line arguments. Compares keywords (function names) with user input.
 *  Initializes data_processing structure.
 *
 *  Assigns an option to the structure depending on what function the user wrote on the command line.
 *
 *  Checks if the user entered the arguments after the name of the functions correctly
 *  (check for natural numbers, check that one argument is greater / greater than or equal to / less than another).
 *
 *  Counts the number of arguments, which belongs to functions,
 *  in order to compare it with the total number of arguments args and return an error if this number does not match
 *
 * @param is_dlm
 * 	  If it is equal to 1, it means that a line with separators has been entered and the parsing of arguments starts from the 3rd argument.
 *    Otherwise, the parsing of arguments starts from the 1st argument.
 * @return error code or 0 if success
 */
int dpf_init(int argc, char* argv[], data_processing* daproc, char is_dlm)
{
    //region variables
    int position = (is_dlm) ? 3 : 1;
    int first_arg = position;
    /**
     * By default it means processing only last row
     */
    daproc->selection.from = 0;
    daproc->selection.to = 0;
    for(int i = 0; i < MAX_ROW_LENGTH; i++) daproc->str[i] = 0;
    int k = 0;
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
    daproc->arg_count = 0;
    //endregion

    while(position < argc)
    {
        if(position == first_arg)
        {
            if(scmp(argv[position], "rows"))
            {
                if(position+3 < argc)
                {
                    if((daproc->selection.from = atoi(argv[position+1])) - atof(argv[position+1]) ||
                       (daproc->selection.to = atoi(argv[position+2])) - atof(argv[position+2]))
                    {
                        return NON_INT_ARGUMENTS_ERROR;
                    }

                    if(daproc->selection.from  < 0 || daproc->selection.to < 0)
                        return SELECTION_IOOF_ERROR;

                    if(!daproc->selection.from)
                    {
                        if(argv[position+1][0] != '-' || argv[position+1][1] != '\0')
                            return SELECTION_IOOF_ERROR;
                    }
                    if(!daproc->selection.to)
                    {
                        if(argv[position+2][0] != '-' || argv[position+2][1] != '\0')
                            return SELECTION_IOOF_ERROR;
                    }

                    if(!daproc->selection.from && daproc->selection.to)
                        return SELECTION_IOOF_ERROR;
                    if(daproc->selection.from && daproc->selection.to && (daproc->selection.from > daproc->selection.to))
                        return SELECTION_IOOF_ERROR;

                    daproc->selection.rs_option = ROWS;
                    daproc->arg_count+=3;
                    position+=3;
                    continue;
                }else return TOO_FEW_ARGS_AFTER_SELECTION;
            }else if(scmp(argv[position], "beginswith"))
                    daproc->selection.rs_option = BEGINSWITH;
                else if(scmp(argv[position], "contains")) daproc->selection.rs_option = CONTAINS;

            if(daproc->selection.rs_option == BEGINSWITH || daproc->selection.rs_option == CONTAINS)
            {
                if(position+3 < argc)
                {
                    if((daproc->selection.from = atoi(argv[position+1])) - atof(argv[position+1]))
                        return NON_INT_ARGUMENTS_ERROR;

                    if(daproc->selection.from <= 0)
                        return SELECTION_IOOF_ERROR;

                    while(argv[position+2][k])
                    {
                        if(k == CELL_LENGTH)
                            return SELECTION_TOO_LONG_PATTERN_ERROR;
                        daproc->selection.pattern[k] = argv[position+2][k];
                        daproc->selection.pattern_len++;
                        k++;
                    }
                    daproc->arg_count+=3;
                    position++;
                    continue;
                }else return WRONG_NUMBER_OF_ARGS;
            }
        }
        if(scmp(argv[position], "tolower")) daproc->dpf_option = TOLOWER;
        else if(scmp(argv[position], "toupper")) daproc->dpf_option = TOUPPER;
            else if(scmp(argv[position], "round")) daproc->dpf_option = ROUND;
                else if(scmp(argv[position], "int")) daproc->dpf_option = INT;

        if(daproc->dpf_option >= TOLOWER && daproc->dpf_option<= INT)
        {
            if(position+1 == argc-1)
            {
                if(atof(argv[position+1]) - atoi(argv[position+1]))
                    return NON_INT_ARGUMENTS_ERROR;
                if((daproc->number1 = atoi(argv[position+1])) <= 0)
                    return DPF_IOOF_ERROR;
            } else return WRONG_NUMBER_OF_ARGS;
            daproc->arg_count+=2;
            break;
        }
        if(scmp(argv[position], "cset")) daproc->dpf_option = CSET;
        else if(scmp(argv[position], "copy")) daproc->dpf_option = COPY;
            else if(scmp(argv[position], "swap")) daproc->dpf_option = SWAP;
                else if(scmp(argv[position], "move")) daproc->dpf_option = MOVE;
                    else if(scmp(argv[position], "csum")) daproc->dpf_option = CSUM;
                        else if(scmp(argv[position], "cavg")) daproc->dpf_option = CAVG;
                            else if(scmp(argv[position], "cmin")) daproc->dpf_option = CMIN;
                                else if(scmp(argv[position], "cmax")) daproc->dpf_option = CMAX;
                                    else if(scmp(argv[position], "ccount")) daproc->dpf_option = CCOUNT;
                                        else if(scmp(argv[position], "cseq")) daproc->dpf_option = CSEQ;
                                            else if(scmp(argv[position], "rsum")) daproc->dpf_option = RSUM;
                                                else if(scmp(argv[position], "ravg")) daproc->dpf_option = RAVG;
                                                    else if(scmp(argv[position], "rmin")) daproc->dpf_option = RMIN;
                                                        else if(scmp(argv[position], "rmax")) daproc->dpf_option = RMAX;
                                                            else if(scmp(argv[position], "rcount")) daproc->dpf_option = RCOUNT;
                                                                else if(scmp(argv[position], "rseq")) daproc->dpf_option = RSEQ;

        if(daproc->dpf_option == CSET)
        {
            if(argc-1 != position+2) return WRONG_NUMBER_OF_ARGS;
            if((atof(argv[position+1]) - atoi(argv[position+1]))) return NON_INT_ARGUMENTS_ERROR;
            if((daproc->number1 = atoi(argv[position+1])) <= 0) return DPF_IOOF_ERROR;

            strcpy(daproc->str, argv[position+2]);
            daproc->arg_count+=3;
            break;
        }
        else if(daproc->dpf_option >= COPY && daproc->dpf_option <= SWAP)
            {
                if(argc-1 != position+2) return WRONG_NUMBER_OF_ARGS;

                if((atof(argv[position+1]) - (daproc->number1 = atoi(argv[position+1]))) ||
                   (atof(argv[position+2]) - (daproc->number2 = atoi(argv[position+2]))))
                {
                    return NON_INT_ARGUMENTS_ERROR;
                }

                if(daproc->number1 <= 0 || daproc->number2  <= 0) return DPF_IOOF_ERROR;
                daproc->arg_count+=3;
                break;
            }else if(daproc->dpf_option >= CSUM && daproc->dpf_option < CSEQ)
                {
                    if(argc-1 != position+3) return WRONG_NUMBER_OF_ARGS;
                    if((daproc->number1 = atoi(argv[position+1])) - atof(argv[position+1]) ||
                       (daproc->number2 = atoi(argv[position+2])) - atof(argv[position+2]) ||
                       (daproc->number3 = atoi(argv[position+3])) - atof(argv[position+3]))
                    {
                        return NON_INT_ARGUMENTS_ERROR;
                    }

                    if(daproc->number1 <= 0 || daproc->number2 <= 0 || daproc->number3 <= 0) return DPF_IOOF_ERROR;

                    if(daproc->number2 > daproc->number3 ||
                       (daproc->number1 >= daproc->number2 &&
                        daproc->number1 <= daproc->number3))
                    {
                        return DPF_IOOF_ERROR;
                    }
                    daproc->arg_count+=4;
                    break;
                }else if(daproc->dpf_option >= CSEQ && daproc->dpf_option <= RSEQ)
                    {
                        if(daproc->dpf_option == CSEQ)
                        {
                            if(argc-1 != position+3) return WRONG_NUMBER_OF_ARGS;
                            daproc->arg_count+=4;
                        }else if(argc-1 != position+4)
                            {
                                return WRONG_NUMBER_OF_ARGS;
                            }

                        if((daproc->number1 = atoi(argv[position+1])) - atof(argv[position+1]) ||
                           (daproc->number2 = atoi(argv[position+2])) - atof(argv[position+2]))
                        {
                            return NON_INT_ARGUMENTS_ERROR;
                        }

                        if(daproc->number1 <= 0 || daproc->number2 <= 0) return DPF_IOOF_ERROR;

                        if(daproc->dpf_option == CSEQ)
                        {
                            if(daproc->number1 > daproc->number2) return DPF_IOOF_ERROR;
                            daproc->number4 = atof(argv[position+3]);
                        }
                        if(daproc->dpf_option == RSEQ)
                        {
                            if((daproc->number3 = atoi(argv[position+3])) - atof(argv[position+3])) return NON_INT_ARGUMENTS_ERROR;
                            if(daproc->number3 == 0 && argv[position+3][0] == '-' && !argv[position+3][1])
                            {
                                daproc->number3 = 0;
                            }
                            else
                            {
                                if(daproc->number3 <= 0) return DPF_IOOF_ERROR;
                                if(daproc->number2 > daproc->number3) return DPF_IOOF_ERROR;
                            }
                            daproc->number4 = atof(argv[position+4]);
                            daproc->arg_count+=5;
                        }
                        break;
                    } else if(daproc->dpf_option >= RSUM && daproc->dpf_option <= RCOUNT)
                        {
                            if(argc-1 != position+3) return WRONG_NUMBER_OF_ARGS;

                            if((daproc->number1 = atoi(argv[position+1])) - atof(argv[position+1]) ||
                               (daproc->number2 = atoi(argv[position+2])) - atof(argv[position+2]) ||
                               (daproc->number3 = atoi(argv[position+3])) - atof(argv[position+3]))
                            {
                                return NON_INT_ARGUMENTS_ERROR;
                            }

                            if((daproc->number1 <= 0 || daproc->number2 <= 0 || daproc->number3 <= 0) ||
                               daproc->number2 > daproc->number3)
                            {
                                return DPF_IOOF_ERROR;
                            }
                            daproc->arg_count+=4;
                            break;
                        }
        position++;
    }
    return 0;
}

/**
 *  The string in column number1 will be converted to lowercase.
 */
void tolower_f(row_info *info, data_processing *daproc)
{
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2];

    while(from < info->last_s[daproc->number1-1])
    {
        if(info->cache[from] >= 'A' && info->cache[from] <= 'Z')
            info->cache[from] += 32;
        from++;
    }
}

/**
 *  The string in column number1 will be converted to uppercase.
 */
void toupper_f(row_info *info, data_processing *daproc)
{
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2];

    while(from < info->last_s[daproc->number1-1])
    {
        if(info->cache[from] >= 'a' && info->cache[from] <= 'z')
            info->cache[from] -= 32;
        from++;
    }
}

/**
 *  The string daproc-str will be set in the cell in column number1 * @param info
 */
void cset_f(row_info *info, data_processing *daproc)
{
    //region variables
    int left_b = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int right_b = info->last_s[daproc->number1-1];
    int cell_len = right_b - left_b;
    int len_topast = slen(daproc->str);
    int diff = len_topast - cell_len;
    //endregion

    if(diff > 0)
    {
        for(int j = info->i; j >= right_b ; j--)
            info->cache[j+diff] = info->cache[j];

        info->i += diff;

        for(int j = left_b; j < left_b+len_topast; j++)
            info->cache[j] = 0;
    }else
    {
        if(daproc->number1 != 1)
            left_b++;

        for(int j = left_b; j < right_b; j++ )
            info->cache[j] = 0;
    }
    row_info_init(&(*info), 0);
    left_b = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    right_b = info->last_s[daproc->number1-1];

    for(int j = left_b, k = 0; k < len_topast; k++, j++)
        info->cache[j] = daproc->str[k];
}

/**
 *  In column number1 rounds the number to an integer.
 */
void round_f(row_info *info, data_processing *daproc)
{
    //region variables
    char negative = 0, dot = 0, is_invalid = 0;
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int to = info->last_s[daproc->number1-1];
    int number = 0;
    char number_str[CELL_LENGTH] = {0};
    //endregion

    if(from >= to) return;
    while(from < to && !is_invalid)
    {
        if(!isnumber(info->cache[from]))
        {
            if(info->cache[from] == '-' && !negative)
                negative++;
            else if(info->cache[from] == '.' && !dot)
                    dot++;
                else
                    is_invalid = 1 ;
        }
        number_str[number++] = info->cache[from++];
    }

    if(is_invalid) return;

    if(atof(number_str) - atoi(number_str) >= 0.5)
        sprintf(daproc->str, "%d", atoi(number_str)+1);
    else
        sprintf(daproc->str, "%d", atoi(number_str));

    cset_f(&(*info), &(*daproc));
}

/**
 *  Removes the decimal part of the number in column number1.
 */
void int_f(row_info *info, data_processing *daproc)
{
    //region variables
    char negative = 0, dot = 0, is_invalid = 0;
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int to = info->last_s[daproc->number1-1];
    int number = 0;
    char number_str[CELL_LENGTH] = {0};
    //endregion

    if(from >= to) return;
    while(from < to && !is_invalid)
    {
        if(!isnumber(info->cache[from]))
        {
            if(info->cache[from] == '-' && !negative)
                negative++;
            else if(info->cache[from] == '.' && !dot)
                    dot++;
                else
                    is_invalid = 1 ;
        }
        number_str[number++] = info->cache[from++];
    }

    if(is_invalid) return;
    sprintf(daproc->str, "%d", atoi(number_str));
    cset_f(&(*info), &(*daproc));
}

/**
 *  Copies informaton from one column to another
 */
void copy_f(row_info *info, data_processing *daproc)
{
    int j = 0;
    int from = (daproc->number2 != 1) ? info->last_s[daproc->number2-2]+1 : 0;
    int to = info->last_s[daproc->number2-1];

    while(from < to)
        daproc->str[j++] = info->cache[from++];

    cset_f(&(*info), &(*daproc));
    j=0;
    while(daproc->str[j]) daproc->str[j++] = 0;
}

/**
 *  Swaps two columns
 */
void swap_f(row_info *info, data_processing *daproc)
{
    if(daproc->number1 == daproc->number2) return;
    //region variables
    int j = 0;
    int from = (daproc->number2 != 1) ? info->last_s[daproc->number2-2]+1 : 0;
    int to = info->last_s[daproc->number2-1];
    char temp_col[MAX_ROW_LENGTH] = {0};
    int temp_col1;
    //endregion

    while(from < to)
        temp_col[j++] = info->cache[from++];

    j=0; // now working with the first column
    from = (daproc->number1 != 1) ? info->last_s[daproc->number1-2]+1 : 0;
    to = info->last_s[daproc->number1-1];

    while(from < to)
        daproc->str[j++] = info->cache[from++];

    temp_col1 = daproc->number1;
    daproc->number1 = daproc->number2;
    cset_f(&(*info), &(*daproc));

    daproc->number1 = temp_col1;
    while(daproc->str[j]) daproc->str[j++] = 0;

    j=0;
    while(daproc->str[j])
    {
        daproc->str[j] = temp_col[j];
        j++;
    }

    cset_f(&(*info), &(*daproc));
    j=0;
    while(daproc->str[j]) daproc->str[j++] = 0;
}

/**
 * Moves the daproc.column1 before the daproc.column2
 */
void move_f(row_info *info, data_processing *daproc)
{
    if(daproc->number1 == daproc->number2-1) return;
    //region variables
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int to = info->last_s[daproc->number1-1];
    char string[MAX_ROW_LENGTH] = {0};
    int k = 0;
    int j = 0;
    int temp_column;
    //endregion
    while(from < to) string[k++] = info->cache[from++];

    icol_f(daproc->number2, &(*info));
    while(j < k)
    {
        daproc->str[j] = string[j];
        j++;
    }

    temp_column = daproc->number1;
    daproc->number1 = daproc->number2;

    cset_f(&(*info), &(*daproc));
    daproc->number1 = temp_column;

    dcol_f(daproc->number1, &(*info));
    k = 0;
    while(k <= j) daproc->str[k++] = 0;
}

/**
 * A cell representing the sum of cell values on the same row from number2 to number3 inclusive
 *  will be stored in the cell in number1
 *   (number2 <= number3, number1 must not belong to the interval <number1; number2>).
 */
void csum_f(row_info *info, data_processing *daproc)
{
    //region variables
    char result[CELL_LENGTH] = {0};
    int from = 0;
    int to = 0;
    int k = 0;
    char switcher = 0;
    char dot = 0;
    char negative = 0;
    info->arithmetic.sum = 0;
    //edregion


    for(int j = daproc->number2; j <= daproc->number3; j++, k=0, negative=0, dot=0, switcher=0)// go through all cells in the row
    {
        if(j != 1) from = info->last_s[j-2]+1;
        to = info->last_s[j-1];

        while(result[k]) result[k++] = 0;
        k=0;

        while(from < to)
        {
            if(!isnumber(info->cache[from]))
            {
                if(info->cache[from] == '-' && !negative)
                {
                    negative++;
                } else if(info->cache[from] == '.' && !dot)
                    {
                        dot++;
                    } else
                    {
                        switcher = 1;
                        break;
                    }
            }
            result[k++] = info->cache[from++];
        }
        if(switcher) continue;

        info->arithmetic.sum += atof(result);
    }
    sprintf(daproc->str,"%g",info->arithmetic.sum);
    cset_f(&(*info), &(*daproc));
}

/**
 *  Similar to csum, but the resulting value represents the arithmetic mean of the values.
 */
void cavg_f(row_info *info, data_processing *daproc)
{
    //region variables
    char switcher = 0, dot = 0, negative = 0;
    info->arithmetic.sum = 0;
    int cols_with_nums = 0, from = 0, to = 0, k = 0;
    char result[CELL_LENGTH] = {0};
    //edregion

    for(int j = daproc->number2; j <= daproc->number3; j++, k=0, negative=0, dot=0, switcher=0)// go through all cells in the row
    {
        if(j != 1) from = info->last_s[j-2]+1;
        to = info->last_s[j-1];

        while(result[k]) result[k++] = 0;
        k=0;

        while(from < to)
        {
            if(!isnumber(info->cache[from]))
            {
                if(info->cache[from] == '-' && !negative)
                {
                    negative++;
                } else if(info->cache[from] == '.' && !dot)
                    {
                        dot++;
                    } else
                    {
                        switcher = 1;
                        break;
                    }
            }
            result[k++] = info->cache[from++];
        }
        if(switcher) continue;
        else cols_with_nums++;

        info->arithmetic.sum += atof(result);
    }
    sprintf(daproc->str,"%g",info->arithmetic.sum/cols_with_nums);
    cset_f(&(*info), &(*daproc));
}

/**
 *  Similar to csum, but the resulting value represents the smallest value found.
 */
void cmin_f(row_info *info, data_processing *daproc)
{
    //region variables
    char negative = 0, switcher = 0, dot = 0;
    int from = 0, to=0, k=0;
    float min = 0;
    info->arithmetic.sum = 0;
    //edregion

    for(int j = daproc->number2; j <= daproc->number3; j++, k=0, negative=0, dot=0, switcher=0)
    {
        if(j != 1) from = info->last_s[j-2]+1;
        to = info->last_s[j-1];

        while(from < to)
        {
            if(!isnumber(info->cache[from]))
            {
                if(info->cache[from] == '-' && !negative)
                {
                    negative++;
                } else if(info->cache[from] == '.' && !dot)
                    {
                        dot++;
                    } else
                    {
                        switcher = 1;
                        break;
                    }
            }
            daproc->str[k++] = info->cache[from++];
        }
        if(switcher) continue;
        k=0;
        if(j == daproc->number2)
        {
            min = atof(daproc->str);
            info->arithmetic.sum = min;
        }else
        {
            info->arithmetic.sum = atof(daproc->str);
            if(info->arithmetic.sum < min) min = info->arithmetic.sum;
        }
        while(daproc->str[k]) daproc->str[k++] = 0;
    }
    sprintf(daproc->str, "%g", min);
    cset_f(&(*info), &(*daproc));
}

/**
 *  Similar to csum, but the resulting value represents the arithmetic mean of the values.
 */
void cmax_f(row_info *info, data_processing *daproc)
{
    //region variables
    char negative = 0, switcher = 0, dot = 0;
    int from = 0, to=0, k=0;
    float max = 0;
    info->arithmetic.sum = 0;
    //edregion

    for(int j = daproc->number2; j <= daproc->number3; j++, k=0, negative=0, dot=0, switcher=0)
    {
        if(j != 1) from = info->last_s[j-2]+1;
        to = info->last_s[j-1];

        while(from < to)
        {
            if(!isnumber(info->cache[from]))
            {
                if(info->cache[from] == '-' && !negative)
                {
                    negative++;
                } else if(info->cache[from] == '.' && !dot)
                    {
                        dot++;
                    } else
                    {
                        switcher = 1;
                        break;
                    }
            }
            daproc->str[k++] = info->cache[from++];
        }
        if(switcher) continue;
        k=0;
        if(j == daproc->number2)
        {
            max = atof(daproc->str);
            info->arithmetic.sum = max;
        }else
        {
            info->arithmetic.sum = atof(daproc->str);
            if(info->arithmetic.sum > max) max = info->arithmetic.sum;
        }
        while(daproc->str[k]) daproc->str[k++] = 0;
    }
    sprintf(daproc->str, "%g", max);
    cset_f(&(*info), &(*daproc));
}

/**
 *  ccount column1 column2 column3 -
 *  The resulting value represents the number of non-empty values of the given cells.
 */
void ccount_f(row_info *info, data_processing *daproc)
{
    for(int j = daproc->number2; j <= daproc->number3; j++)
    {
        if(j == 1)
        {
            if(0 == info->last_s[j-1]) info->arithmetic.empties++;
        }else
        {
            if(info->last_s[j-2] >= info->last_s[j-1]) info->arithmetic.empties++;
        }
    }
    sprintf(daproc->str, "%llu", daproc->number3 - daproc->number2+1 - (info->arithmetic.empties));
    cset_f(&(*info), &(*daproc));
}

/**
 *  Cseq column1 column2 column3 - inserts gradually increasing numbers (by one)
 *  starting with the value column3 into the cells in column1 to column2 inclusive
 */
void cseq_f(row_info *info, data_processing *daproc)
{
    info->arithmetic.sum = daproc->number4;
    int k = 0;
    int column1 = daproc->number1;

    for(int i = column1; i <= daproc->number2; i++,info->arithmetic.sum++, k=0 )
    {
        daproc->number1 = i;
        sprintf(daproc->str, "%g", (float)info->arithmetic.sum);
        cset_f(&(*info),&(*daproc));
        while(daproc->str[k]) daproc->str[k++] = 0;
    }
    daproc->number1 = column1;
}

/**
 *  Inserts the sum of cell values in number1 on rows number2 to number3 inclusive
 *  into the cell in number1 on row number3+1
 */
void rsum_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->number3)
    {
        sprintf(daproc->str, "%g", info->arithmetic.sum);
        cset_f(&(*info),&(*daproc));
    }
    //region variables
    char negative = 0, dot = 0, invalid_row = 0;
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int to = info->last_s[daproc->number1-1];
    int k = 0;
    char number_str[CELL_LENGTH] = {0};
    //endregion

    while(from < to)
    {
        if(!(info->cache[from]))
        {
            if(info->cache[from] == '-' && !negative)
                negative++;
            else if(info->cache[from] == '.' && !dot)
                    dot++;
                else
                    invalid_row = 1;
        }
        number_str[k++] = info->cache[from++];
    }
    if(invalid_row) return;

    info->arithmetic.sum += atof(number_str);
}

/**
 *  Calculates the arithmetic mean of numbers in columns number1 from row number2 to row number3 inclusive
 *  and inserts thsi value the cell in number1 on row number3 +1.
 */
void ravg_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->number3)
    {
        sprintf(daproc->str, "%g", info->arithmetic.sum / info->arithmetic.valid_row);
        cset_f(&(*info),&(*daproc));
    }
    //region variables
    char negative = 0, dot = 0, invalid_row = 0;
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int to = info->last_s[daproc->number1-1];
    int k = 0;
    char number_str[CELL_LENGTH] = {0};
    //endregion

    while(from < to)
    {
        if(!(info->cache[from]))
        {
            if(info->cache[from] == '-' && !negative)
                negative++;
            else if(info->cache[from] == '.' && !dot)
                    dot++;
                else
                    invalid_row = 1;
        }
        number_str[k++] = info->cache[from++];
    }
    if(invalid_row) return;
    info->arithmetic.valid_row++;

    info->arithmetic.sum += atof(number_str);
}

/**
 *  Finds the minimum number in columns from row number2 to number inclusive
 *  into the cell in number1 on row number3 +1.
*/
void rmin_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->number3)
    {
        sprintf(daproc->str, "%g", info->arithmetic.min_max);
        cset_f(&(*info),&(*daproc));
    }
    //region variables
    char negative = 0, dot = 0, invalid_row = 0;
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int to = info->last_s[daproc->number1-1];
    int k = 0;
    char number_str[CELL_LENGTH] = {0};
    //endregion

    while(from < to)
    {
        if(!(info->cache[from]))
        {
            if(info->cache[from] == '-' && !negative)
                negative++;
            else if(info->cache[from] == '.' && !dot)
                    dot++;
                else
                    invalid_row = 1;
        }
        number_str[k++] = info->cache[from++];
    }
    if(invalid_row) return;

    if(daproc->number2 == info->current_row+1)
    {
        info->arithmetic.min_max = atof(number_str);
        info->arithmetic.sum = info->arithmetic.min_max;
    }
    info->arithmetic.sum = atof(number_str);
    if(info->arithmetic.sum < info->arithmetic.min_max)
        info->arithmetic.min_max = info->arithmetic.sum;
}

/**
 *  Inserts the maximum of cell values in column number1 on rows number2 to number3 inclusive
 *  into the cell in number1 on row number3 +1.
*/
void rmax_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->number3)
    {
        sprintf(daproc->str, "%g", info->arithmetic.min_max);
        cset_f(&(*info),&(*daproc));
    }
    //region variables
    char negative = 0, dot = 0, invalid_row = 0;
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int to = info->last_s[daproc->number1-1];
    int k = 0;
    char number_str[CELL_LENGTH] = {0};
    //endregion

    while(from < to)
    {
        if(!(info->cache[from]))
        {
            if(info->cache[from] == '-' && !negative)
                negative++;
            else if(info->cache[from] == '.' && !dot)
                    dot++;
                else
                    invalid_row = 1;
        }
        number_str[k++] = info->cache[from++];
    }
    if(invalid_row) return;

    if(daproc->number2 == info->current_row+1)
    {
        info->arithmetic.min_max = atof(number_str);
        info->arithmetic.sum = info->arithmetic.min_max;
    }
    info->arithmetic.sum = atof(number_str);
    if(info->arithmetic.sum > info->arithmetic.min_max)
        info->arithmetic.min_max = info->arithmetic.sum;
}

/**
 *  Inserts the number  of non-empty columns number1 from rows number2 to number3 inclusive
 *  into the cell in number1 on row number3 +1.
*/
void rconut_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->number3)
    {
        sprintf(daproc->str, "%llu", (unsigned long long)(daproc->number3 - daproc->number2+1)-info->arithmetic.empties);
        cset_f(&(*info),&(*daproc));
    }
    int from = (daproc->number1 == 1) ? 0 : info->last_s[daproc->number1-2]+1;
    int to = info->last_s[daproc->number1-1];

    if(from >= to) info->arithmetic.empties++;
}

/**
 *  Inserts ascending numbers in the cells of each row  from from the certain range.
 */
void rseq_f(row_info *info, data_processing *daproc)
{
    if(daproc->number3)
    {
        // range is from daroc->number2 to daproc->number3
        if(info->current_row+1 >= daproc->number2 && info->current_row+1 <= daproc->number3)
        {
            sprintf(daproc->str, "%g", daproc->number4);
            cset_f(&(*info),&(*daproc));
            daproc->number4++;
        }
    }else // if the user entered a hyphen
    {
        if(info->current_row+1 >= daproc->number2)
        {
            sprintf(daproc->str, "%g", daproc->number4);
            cset_f(&(*info),&(*daproc));
            daproc->number4++;
        }
    }
}
//endregion

/**
 *  Determines what functions the user has entered using the switch statment .
 *
 *  Determines if the user has entered a row selection.
 *
 *  Checks if the parameters do not exceed the number of columns.
 *
 *  Аssigns a function pointer to the function that should be called.
 *
 *  Then calls the function
 */
void dpf_call(row_info *info, data_processing *daproc)
{
    if(info->num_of_cols < daproc->number1) return;
    if(info->cache[0] == EOF) return;

    //region Variables
    int k = 0;
    int from = 0;
    int to = 0;
    char string[CELL_LENGTH] = {0};

    void (*function_to_call)(row_info *info ,data_processing *daproc);
    //endregion
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
            function_to_call = &cset_f;
            break;
        case CSUM:
            function_to_call = &csum_f;
            break;
        case CAVG:
            function_to_call = &cavg_f;
            break;
        case CMIN:
            function_to_call = &cmin_f;
            break;
        case CMAX:
            function_to_call = &cmax_f;
            break;
        case CCOUNT:
            function_to_call = &ccount_f;
            break;
        case CSEQ:
            function_to_call = &cseq_f;
            break;
        case RSUM:
            function_to_call = &rsum_f;
            break;
        case RAVG:
            function_to_call = &ravg_f;
            break;
        case RMIN:
            function_to_call = &rmin_f;
            break;
        case RMAX:
            function_to_call = &rmax_f;
            break;
        case RCOUNT:
            function_to_call = &rconut_f;
            break;
        case RSEQ:
            function_to_call = &rseq_f;
            break;
        case NO_DPF:
            return;
    }

    if(daproc->dpf_option >= TOLOWER && daproc->dpf_option <= INT)
    {
        //I moved the check of the suitability of the cell from here to the function,
        //thereby reducing the number of iterations and, possibly, the execution time of the program.
        //Now the check for the suitability of the cell(this means
        // there must be no text in the cell for arithmetic functions to be performed)
        //is in functions themselves.
        //The program has a code repetition, but it works faster due to fewer iterations
        if(daproc->number1 > info->num_of_cols) return;
    }
    else if(daproc->dpf_option >= CSUM && daproc->dpf_option <= CCOUNT)
        {
            // --//--
            if(daproc->number3 > info->num_of_cols ||
               daproc->number2 > info->num_of_cols)
            {
                return;

            }
        }else if(daproc->dpf_option >= RSUM && daproc->dpf_option <= RCOUNT)
            {
                // --//--
                if(info->current_row+1 < daproc->number2 || // +1 because rows are counted from 0.
                   info->current_row > daproc->number3 || // If we are out of range, do nothing
                   info->num_of_cols < daproc->number1)
                {
                    return;
                }
            }else if(daproc->dpf_option >= COPY && daproc->dpf_option <= SWAP)
                {
                    if((daproc->number1 == daproc->number2) || // in this case we dont need to call the the function besause it changes nothing
                       daproc->number1 > info->num_of_cols ||
                       daproc->number2 > info->num_of_cols)
                    {
                        return;
                    }
                }

    switch(daproc->selection.rs_option)
    {
        case ROWS:
            if(!daproc->selection.from && !daproc->selection.to)
            {
                if(info->is_lastrow)
                    function_to_call(&(*info), &(*daproc));
            } else if(daproc->selection.from && !daproc->selection.to)
                {
                    if(info->current_row+1 >= daproc->selection.from)
                        function_to_call(&(*info), &(*daproc));
                } else if(info->current_row+1 >= daproc->selection.from && info->current_row+1 <= daproc->selection.to)
                    {
                        function_to_call(&(*info), &(*daproc));
                    }
            break;
        case BEGINSWITH:
            if(daproc->selection.from > info->num_of_cols) return;

            from = (daproc->selection.from == 1) ? 0 : info->cache[info->last_s[daproc->selection.from-2]]+1;
            to = info->last_s[daproc->selection.from-1];

            if(from >= to) return;

            while(k < daproc->selection.pattern_len)
            {
                if(info->cache[from+k] != daproc->selection.pattern[k]) return;
                k++;
            }
            function_to_call(&(*info), &(*daproc));
            break;
        case CONTAINS:
            from = (daproc->selection.from == 1) ? 0 : info->cache[info->last_s[daproc->selection.from-2]]+1;
            to = info->last_s[daproc->selection.from-1];

            if(from >= to) return;

            while(from < to) string[k++] = info->cache[from++];

            if(find(daproc->selection.pattern,string)) function_to_call(&(*info), &(*daproc));
            break;

        case NO_SELECTION:
            function_to_call(&(*info), &(*daproc));
            return;
    }
}

/**
 *  Цalls table editing functions that can return an error code.
 *  Functions are specifically called in this order so as not to conflict with each other.
 */
int tef_call(row_info *info, table_edit *tedit)
{
    int j;
    int exit_code = 0;

    for(j = tedit->d_col.called-1; j >= 0 ; j--)
    {
        if((exit_code = dcol_f(tedit->d_col.victims[j], &(*info)))) return exit_code;
    }
    for(j = 0; j < (int)tedit->a_col.called; j++)
    {
        if((exit_code = acol_f(&(*info)))) return exit_code;
    }
    for(j = tedit->i_col.called-1; j >= 0 ; j--)
    {
        if((exit_code = icol_f(tedit->i_col.victims[j], &(*info)))) return exit_code;
    }
    for(j = 0; j < (int)tedit->d_row.called; j++)
    {
        drow_f(tedit->d_row.victims[j], &(*info));
    }
    for(j = 0; j < (int)tedit->i_row.called; j++)
    {
        irow_f(tedit->i_row.victims[j], &(*info));
    }
    for(j = 0; j < (int)tedit->a_row.called; j++)
    {
        arow_f(&(*info));
    }
    return exit_code;
}

/**
 *  First checks the number of arguments so that it does not exceed 100.
 *
 *  This is followed by the initialization of separators, if the user entered them.
 *  Otherwise, a space is considered a separator.
 *
 *  Next, the functions that the user entered in the command line are initialized.
 *  The functions and their arguments are checked for correctness.
 *  In case of invalid arguments, an appropriate error code is returned.
 *
 *
 *	Next, the row from the stdin is scanned to array called cache.
 *	After scanning one row, functions are called to process the table or data.
 *	Functions can return an appropriate error code.
 *
 *	Next, the row is written to stdout.
 *
 *  The file is processed line by line.
 *
 * @param argc Number of arguments of command line
 * @param argv Values arguments of command line
 * @return 0 if success otherwise error code
 */
int cache_init(int argc, char* argv[])
{
    if(argc > MAX_NUMBER_OF_ARGUMENTS) return TOO_MUCH_ARGUMENTS_ERROR;

    //region variables
    row_info info;
    info.i = 0;
    info.current_row = 1;
    info.is_lastrow = 0;
    info.arithmetic.valid_row = 1;
    info.arithmetic.sum = 0;
    info.arithmetic.empties = 0;
    table_edit tedit;
    data_processing daproc;

    char first_symbol = 0, is_dlm = 0;
    /**
     * By defult it is 0, but functons can change this value. In this case program will be terminated
     */
    int exit_code = 0;
    //endregion

    if(argc > 2 && !scmp(argv[1], "-d"))
    {
        info.seps.number_of_seps = 1;
        info.seps.separators[0] = ' ';
    }else if(argc > 3 && scmp(argv[1], "-d"))
        {
            is_dlm = 1;
            exit_code = separators_init(argv[2], &info);
            if(exit_code) return exit_code;
        } else return TOO_FEW_ARGS_ERROR;

    if((exit_code = tef_init(argc, argv, &tedit, is_dlm))) return exit_code;
    if((exit_code = dpf_init(argc, argv, &daproc, is_dlm))) return exit_code;
    if(daproc.dpf_option == NO_DPF)
    {
        if(is_dlm)
        {
            if(tedit.arg_count+2 != argc-1)
                return EXCESS_ARGUMENTS_ERROR;
        }else
        {
            if(tedit.arg_count != argc-1)
                return EXCESS_ARGUMENTS_ERROR;
        }
    }else
    {
        if(is_dlm)
        {
            if(daproc.arg_count+2 != argc-1)
                return EXCESS_ARGUMENTS_ERROR;
        }else
        {
            if(daproc.arg_count != argc-1)
                return EXCESS_ARGUMENTS_ERROR;
        }
    }

    /**
     * Scan the file line by line
     * if we find \n char go to the function parsing, parse functons, give line to the functions
     *
     */
    do
    {
        if(!info.i && info.current_row) info.cache[info.i] = first_symbol;
        else info.cache[info.i] = getchar();

        if(info.i == MAX_ROW_LENGTH-1 && info.cache[info.i] != 10) return MAX_LENGTH_REACHED;

        if(info.cache[info.i] == 10 || info.cache[info.i] == EOF)
        {

            if(info.cache[info.i] != EOF)
            {
                info.row_seps.number_of_seps = 0;
                if(row_info_init(&info, 1)) return MAX_CELL_LENGHT_ERROR;

                if((first_symbol = getchar()) == EOF) info.is_lastrow = 1;
            }

            if(info.i == 1 && info.cache[info.i] == EOF) return EMPTY_STDIN_ERROR;

            if((exit_code = tef_call(&info, &tedit))) return exit_code;

            dpf_call(&info, &daproc);

            if((exit_code = print(&info)))
            {
                if(exit_code == 1)
                    break;
                return exit_code;
            }
            continue;
        }
        (info.i)++;
    }while(1);
    return 0;
}

int main(int argc, char* argv[])
{
    clock_t begin = clock();

    int exit_code = 0;
    char* error_msg[] = {
            "ERROR: Empty stdin. There is no in input to edit it.\n", //0
            "ERROR: One of the rows is too long. Try to make in shorter.\n", //1
            "ERROR: Separators you entered are not supported by the program to prevent undetermined behaviour."
            ". They mustnt be numbers, letters or symbols from 0 to 10 ASCII values. \n", //2
            "WARNING: There is different number of separators in differrent lines,"
            " program may not work properly. Check each line for number of separators or make sure "
            "you entered all possible separators in DELIM. \n", //3
            "ERROR: The program supports at most one row selector. Enter less selectors.\n", //5
            "ERROR: Too much arguments. Program supports at most 100 arguments. Enter less arguments.\n", //6
            "ERROR: Wrong parameters after dcol. Enter parameter greater than 0 after dcol.\n",//7
            "ERROR: Wrong parameters after dcols. 1-st parameter must be less than 2-nd."
            " Both must be greater than 0.\n",//8
            "ERROR: Wrong parameters after drow. Enter parameter greater than 0 after drow.\n",//9
            "ERROR: Wrong parameters after drows. 1-st parameter must be less or equal than 2-nd. "
            "Both must be greater than 0.\n",//10
            "ERROR: Wrong parameters after irow. Enter parameter greater than 0 after irow.\n",//11
            "ERROR: Wrong parameters after icol. Enter parameter greater than 0 after icol.\n",//12
            "ERROR: Column you entered is out of range.\n",//13
            "ERROR: Too few arguments after row selection command. Enter more arguments.\n", //14
            "ERROR: Wrong number of arguments after data processing command. Enter valid number of arguments.\n", //15
            "ERROR: Index after data processing command is out of range.\n",//16
            "ERROR: Index after row selection is out of range.\n",
            "ERROR: Too few functions or delim string in command line.\n",//17
            "ERROR: Pattern you entered after row selection parameter is longer than max row lenght\n",// selection to l p wtf
            "ERROR: Non integer arguments. Arguments must be without dcimal part. Enter the integer arguments\n",
            "ERROR: One of the rows is too long.Maximum supported cel length is 100\n",
            "ERROR: No function was called. Enter a function name and its arguments.\n",
            "ERROR: Extra command line arguments found. Enter valid command line arguments"

    };

    exit_code = cache_init(argc, argv);

    if(exit_code > 1)
        fprintf(stderr, "\nErrNo %d %s  ", exit_code, error_msg[exit_code-2] );

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    fprintf(stderr, "\nTotal time: %lfs.\n", time_spent);

    return exit_code;
}
