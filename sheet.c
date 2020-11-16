/**
 * Project 1 - simple spredsheet editor
 * Subject IZP 2020/21
 * @Author Skuratovich Aliaksandr xskura01@fit.vutbr.cz
*/

//region Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//endregion

//region defines
#define MAX_ROW_LENGTH          10241
#define CELL_LENGTH             100
#define SEPS_SIZE               42 // 128(ASCII) - 32(first white symbols) - 48(alphabet) - 10(numbers).
#define MAX_NUMBER_OF_ARGUMENTS 100

#define DELETED_SEPARATOR       7
#define SEPARATOR_TO_PRINT      3
#define END_OF_FILE             1
//endregion

//region Enums
enum errors_enum {
    EMPTY_STDIN_ERROR = 2,
    MAX_LENGTH_REACHED,
    UNSUPPORTED_SEPARATORS_ERROR,
    TOO_MUCH_ARGUMENTS_ERROR,
    BAD_ARGUMENTS_ERROR_TEF,
    TOO_FEW_ARGS_AFTER_SELECTION,
    WRONG_NUMBER_OF_ARGS_DP,
    DPF_IOOR_ERROR,
    SELECTION_IOOR_ERROR,
    SELECTION_TOO_LONG_PATTERN_ERROR,
    NON_INT_ARGUMENTS_ERROR,
    MAX_CELL_LENGTH_ERROR,
    TOO_FEW_ARGS_ERROR,
    EXCESS_ARGUMENTS_ERROR,
	DPF_TOO_LONG_PATTERN_ERROR,
	NON_RAT_ARGUMENTS_ERROR,
	CONCAT_MAX_CELL_LEN_ERROR,
	COL_IS_NOT_A_NUM_ERROR
};

typedef enum options_is_str_number_f {
	WHOLE, RATIONAL
} options_is_str_number_f;

// options for icol and row_info_init functions
typedef enum fun_option {
	NO_OPTION,
	CHECK_INSERTED_SEPS,
	CHANGE_ALL_SEPS_TO_THE_FIRST_SEP_FROM_DELIM
} fun_option;

typedef enum dpf_option_enum {
    NO_DPF,
    TOLOWER, TOUPPER, ROUND, INT, // 1 number as parameter
    COPY, MOVE, SWAP, // 2 numbers as parameters
    CSET, // Number and string as parameters
    CSUM, CAVG, CMIN, CMAX, CCOUNT, // 3 numbers as parameters
    CSEQ, // 3 numbers as parameters and one cal be floating pointer or 0
    RSEQ, // 4 numbers as parameters
    RSUM, RAVG, RMIN, RMAX, RCOUNT,
    CONCAT
} dpf_option_enum;

typedef enum sel_option_enum {
    NO_SELECTION,
    ROWS,
    BEGINSWITH, CONTAINS
} sel_option_enum;
//endregion

//region Structures
/**
 * Contains information about separators from delim string
 */
typedef struct seps_struct {
    char            separators[SEPS_SIZE];
    int             number_of_seps;
} seps_struct;

/**
 * Structure for arithmetical functions as
 *  CSUM, CAVG, CMIN, CMAX, CCOUNT, CSEQ, RSUM, RAVG, RMIN, RMAX, RCOUNT,
 */
typedef struct arithm_t {
    float           sum;
    unsigned int    valid_row; // to check if row can be used for arithmetical functions.
    float           min_max; // contains information about min or max nuber
    //if there is a very big table with lots of rows the date type int may not be enough
    unsigned int    empties;
} arithm_t;

/**
 * Contains informatoin about scanned row
 */
typedef struct row_info {
    int             l; // position of the last element of the row, basically represents length,
    int             current_row;
    char            cache[MAX_ROW_LENGTH]; // cache contains one single row of the file
    seps_struct     seps;
    seps_struct     row_seps;
    int             last_s[MAX_ROW_LENGTH]; // array woth positions of right separators of cells
    int             num_of_cols;
    char            is_lastrow;
    arithm_t        arithmetic; // structure contains variables for aritmetic functions
	int             deleted_row; // for make drow function o(1) intead of o(n)
} row_info;

typedef struct interval_t {
	int             x1; // left value
	int             x2; // right value
} interval_t;

typedef struct tef_t {
	interval_t      params[MAX_NUMBER_OF_ARGUMENTS]; // intervals of type <x1, X2>
    int             param_c; // length oof an array above
} tef_t;

typedef struct table_edit {
    int             arg_count_te;
    tef_t           d_col;
    tef_t           i_col;
    tef_t           a_col;
    tef_t           a_row;
    tef_t           i_row;
    tef_t           d_row;
} table_edit;

typedef struct row_sel {
    /**
     * sel_option says which data processing function program will use
     */
    sel_option_enum sel_option;
    int             from;
    int             to;
    int             pattern_len;
    char            pattern[CELL_LENGTH];
} row_sel;

typedef struct data_processing {
    int             arg_count_dp;
    dpf_option_enum dpf_option;       //cset tolower toupper...  1 2 3 4 ..
    int             num1;             // for functions cset, tolower, toupper, round, int, cset, copy, swap, move
    int             num2;             // for functions copy, swap, move
    int             num3;             // for functions csum, cavg, cmax, ccount, cseq, rsum, ravg, rmin, rmax, rcount
    float            num4;            // for rseq function
    char            str[CELL_LENGTH]; // for cset function
    row_sel         sel;
} data_processing;
//endregion


//region Functions
/**
 *  Swaps values of two integer values
 */
void swap(int *x1, int *x2)
{
	*x1 = *x1 ^ *x2;
	*x2 = *x1 ^ *x2;
	*x1 = *x1 ^ *x2;
}
/**
 *  Sorts the array with intervals using the bubble sort.
 *  Removes the intersections of these intervals,
 *  if there are any in different array elements. Inserts 0 0 in place of the deleted interval, because
 *  it is faster iterating over the offset of each array element by 1
 *
 * @param intervals array with the intervals to sort it
 * @param size size of an array
 */
void sort_del_reps( interval_t intervals[MAX_NUMBER_OF_ARGUMENTS], int *size)
{
    interval_t tmp_arr[MAX_NUMBER_OF_ARGUMENTS];
	int new_size = 0;
	// Sort intervals.
    for(int i = 0; i < *size - 1; i++)
    {
        for(int j = i + 1; j < *size; j++)
        {
			if(!intervals[j].x2)
				continue;
			if(intervals[i].x1 > intervals[j].x1 )
            {
				swap(&intervals[i].x1, &intervals[j].x1);
				swap(&intervals[i].x2, &intervals[j].x2);
			} else if(intervals[i].x1 == intervals[j].x1 && intervals[i].x2 > intervals[j].x2 )
			{
				swap(&intervals[i].x1, &intervals[j].x1);
				swap(&intervals[i].x2, &intervals[j].x2);
			}
        }
    }
	
	// merge intervals
	for(int i = 0; i < *size - 1; i++)
	{
		for(int j = i + 1; j < *size; j++ )
		{
			if(intervals[i].x2 + 1 >= intervals[j].x1)
			{
				if(intervals[i].x2 < intervals[j].x2)
					intervals[i].x2 = intervals[j].x2;
				intervals[j].x2 = 0;
			}

		}
	}
	for(int i = 0; i < *size; i++)
	{
		if(intervals[i].x2)
		{
			tmp_arr[new_size].x1 = intervals[i].x1;
			tmp_arr[new_size].x2 = intervals[i].x2;
			new_size++;
		}
	}
	
    for (int i = 0; i < new_size; i++)
	{
		intervals[i] = tmp_arr[i];

	}

	*size = new_size;
}


int slen(char *arr)
{
    int length = 0;
    while(arr[length])
        length++;
	
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
    for(int i = 0; string[i]; ++i)
    {
        for(int j = 0;; ++j)
        {
            if(!pattern[j])
                return 1;
			
            if(string[i + j] != pattern[j])
                break;
        }
    }
    return 0;
}

/**
 *  Binary searches an element in the sorted array.
 *
 * @param arr is an array of integers to find the element in
 * @param left_b left border of an array
 * @param right_b right border of an array
 * @param elem emement to find
 * @return 1 if element elem is in array
 *         0 if there is no element elem in the array
 *
 */
int bin_search(tef_t *tef_strct, int left_b, int right_b, int elem)
{
    if (right_b >= left_b)
	{
        int new_b = left_b + (right_b - left_b) / 2;
  
		if (tef_strct->params[new_b].x1 <= elem && tef_strct->params[new_b].x2 >= elem )
            return 1;
  
		if (tef_strct->params[new_b].x1 > elem) // change to x2 > elem
            return bin_search(tef_strct, left_b, new_b - 1, elem);
  
        return bin_search(tef_strct, new_b + 1, right_b, elem);
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
    if((len1 = slen(string1)) != slen(string2))
        return 0;

    while(len1--)
    {
        if(string1[len1] != string2[len1])
            return 0;
    }
    return 1;
}

char isnumber(char suspect)
{
    return (suspect >= 48 && suspect <= 57) ? 1 : 0;
}

/**
 *   Checs whether a string is a number or contains letters or - is not at the 0 position
 *   or is repeated.
 * @param string String for check
 * @return 0 is the entire string is not a number
 *         1 otherwise
 */
char is_str_number(char *str, options_is_str_number_f option)
{
    int  k = 0;
    char dot = 0;
    while(str[k])
    {
        if(!isnumber(str[k]))
        {
            if(str[k] == '.' && !dot)
				dot++;
			else if(str[k] != '-' || k)
				return 0;
        }
		if(option == WHOLE && (dot || str[0] == '0' || str[0] == '-'))
			return 0;
        k++;
    }
    return 1;
}

void clear_str(char *str)
{
	int i  =  0;
	while(str[i])
		str[i++] = 0;
}
/**
 *  Gets an array of characters, information about the file line and the cell number to be processed.
 *  Zeroes the array of characters.
 *  Goes through the cell and checks if there is a number in the cell.
 *  If the cell contains any other chars than the number itself
 *  (the number can be negative or floating point) returns 0.
 *  If there is only a number in the cell, it returns 1 and writes the number to the array
 *
 * @param dst string where function overwtites column with number
 * @param info contains row with columns and  of right separators of the columns
 * @param column Which column function has to check
 * @return 1 if column is valid
 *         0 if column contains not only a number
 */
char number_validate_str(char *dst, row_info *info, int column)
{
    int from = (column == 1) ? 0 : info->last_s[column - 2] + 1;
    int to = info->last_s[column - 1];
    int k = 0;
    char dot = 0;
	
	clear_str(dst);

    while(from < to)
    {
        if(!isnumber(info->cache[from]))
        {
            if(info->cache[from] == '.' && !dot)
                dot++;
			else if(info->cache[from] != '-' || k)
			{
				return 0;
			}
        }
        dst[k++] = info->cache[from++];
    }
    return 1;
}

//endregion

/**
 * Prints cache to the stdout.
 * If there are characters in the cache that should not be written out
 *  or should be replaced with other characters, it does so.
 *  This is done so that some functions do not conflict with each other.
 * @param string_to_print
 * @param size
 */
void print(int *exit_code, row_info *info)
{
	if(info->deleted_row)
	{
		info->current_row++;
		info->l           = 0;
		info->deleted_row = 0;
		return;
	}else
    {
        int s = 0;
        while(s <= info->l)
        {
            if(info->cache[s] == EOF)
            {
                *exit_code = END_OF_FILE;
                return;
            } else if(info->cache[s] == 0 || info->cache[s] == DELETED_SEPARATOR)
            {
                s++;
                continue;
            } else if(info->cache[s] == SEPARATOR_TO_PRINT)
            {
                putchar(info->seps.separators[0]);
                s++;
                continue;
            }
            putchar(info->cache[s]);
            s++;
        }
		//clear_str(info->cache); not necessary
		
        info->current_row++;
        info->l = 0;
    }
}

//region functions for row_info
/**
 * Initialise sepatrators from delim
 * @param argv2 Delim string - string with entered separators
 * @param info information about unique row.
 * @return returns an error code if the string contains characters that could cause undefined program behavior
 *  otherwise 0
 */
int separators_init(char *argv2, row_info *info)
{
    //region variables
    info->seps.number_of_seps = 0;
    char switcher;
    int j;
    int k = 0;
    //endregion

    while(argv2[k])
    {
        /**
         * Prevent user to enter theese symbols for correct functioninig of the program
         */
        if((argv2[k] >= 'a' && argv2[k] <= 'z') ||
           (argv2[k] >= 'A' && argv2[k] <= 'Z') ||
           (argv2[k] >= '0' && argv2[k] <= '9') ||
		   (argv2[k] <= 31))
        { return UNSUPPORTED_SEPARATORS_ERROR; }
		
        switcher = 0;

        j = k + 1;
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
void row_info_init(int *exit_code, row_info *info, fun_option option)
{
    if(info->cache[0] == EOF) return;
	
    int 		  last_se = 0;
    unsigned char k = 0;

    /**
     * go to the end of the column in this cycle
     * checks every char if is a separator
     * adds position of last separator of each column to an array
     */
    for(int j = 0, clen = 0; j <= info->l; j++, clen++)
    {
        if(option == CHANGE_ALL_SEPS_TO_THE_FIRST_SEP_FROM_DELIM)
        {
            while(k < info->seps.number_of_seps)
            {
                if(info->cache[j] == info->seps.separators[k])
                {
                    info->cache[j] = info->seps.separators[0];
                    info->row_seps.number_of_seps++;
                    break;
                }
                k++;
            }
            k = 0;
        }
        if(info->cache[j] == info->seps.separators[0] ||
		   info->cache[j] == DELETED_SEPARATOR        ||
           (option == CHECK_INSERTED_SEPS && info->cache[j] == SEPARATOR_TO_PRINT))
        {
            clen = 0;
            info->last_s[last_se] = j;
            last_se++;
        } else if(info->cache[j] == 10)
        {
            info->last_s[last_se] = j;
            break;
        }

        if(clen > CELL_LENGTH)
		{
			*exit_code = MAX_CELL_LENGTH_ERROR;
			return;
		}
    }

    // Also num_of_cols is len of array with last separators
    info->num_of_cols = ++last_se;
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
 *
 *  Counts the number of parameters, which belong to functions,
 *  in order to compare it with the total number of command line arguments and return an error if theese numbers do not match

 *  Adds arguments of table edit functions into arrays, so that later these functions can be simply called with arguments.
 *  Also sorts arrays and removes duplicate elements so as not to call the same function multiple times
 *
 *
 * @param is_dlm
 * 	  If it is equal to 1, it means that a line with separators has been entered and the parsing of arguments starts from the 3rd argument.
 *    Otherwise, the parsing of arguments starts from the 1st argument.
 * @return error code or 0 if success
 */
int tef_init(int argc, char *argv[], table_edit *tedit_t, char is_dlm)
{
    //region variables
    int position = (is_dlm) ? 3 : 1;
	
    tedit_t->d_col.param_c = 0;
    tedit_t->d_row.param_c = 0;
    tedit_t->a_col.param_c = 0;
    tedit_t->a_row.param_c = 0;
    tedit_t->i_row.param_c = 0;
    tedit_t->i_col.param_c = 0;
	
    tedit_t->arg_count_te = 0;
    //endregion

    while(position < argc)
    {
		if((tedit_t->d_col.param_c +
			tedit_t->d_row.param_c +
			tedit_t->a_col.param_c +
			tedit_t->a_row.param_c +
			tedit_t->i_row.param_c +
			tedit_t->i_col.param_c) >
			MAX_NUMBER_OF_ARGUMENTS)
		{ return TOO_MUCH_ARGUMENTS_ERROR; }
		
        if(scmp(argv[position], "dcol"))
        {
            if(position + 1 < argc)
            {
                if(!is_str_number(argv[position + 1], WHOLE))
					return BAD_ARGUMENTS_ERROR_TEF;
				
				tedit_t->d_col.params[tedit_t->d_col.param_c].x1 = atoi(argv[position + 1]);
				tedit_t->d_col.params[tedit_t->d_col.param_c].x2 = tedit_t->d_col.params[tedit_t->d_col.param_c].x1;
				
				tedit_t->d_col.param_c++;
                position              += 2;
                tedit_t->arg_count_te += 2;
                continue;
            } else return BAD_ARGUMENTS_ERROR_TEF;
        } else if(scmp(argv[position], "dcols"))
        {
            if(position + 2 < argc)
            {
                if(!is_str_number(argv[position + 1], WHOLE) || !is_str_number(argv[position + 2], WHOLE))
                    return BAD_ARGUMENTS_ERROR_TEF;
				
				tedit_t->d_col.params[tedit_t->d_col.param_c].x1 = atoi(argv[position + 1]);
				tedit_t->d_col.params[tedit_t->d_col.param_c].x2 = atoi(argv[position + 2]);
				
				if(tedit_t->d_col.params[tedit_t->d_col.param_c].x1 >
				   tedit_t->d_col.params[tedit_t->d_col.param_c].x2)
				{return BAD_ARGUMENTS_ERROR_TEF;}

				tedit_t->d_col.param_c++;
                tedit_t->arg_count_te += 3;
                position              += 3;
                continue;
            } else return BAD_ARGUMENTS_ERROR_TEF;
        } else if(scmp(argv[position], "drow"))
        {
            if(position + 1 < argc)
            {
                if(!is_str_number(argv[position + 1], WHOLE)) return BAD_ARGUMENTS_ERROR_TEF;
				
				tedit_t->d_row.params[tedit_t->d_row.param_c].x1 = atoi(argv[position + 1]);
				tedit_t->d_row.params[tedit_t->d_row.param_c].x2 = tedit_t->d_row.params[tedit_t->d_row.param_c].x1;

				tedit_t->d_row.param_c++;
                tedit_t->arg_count_te += 2;
                position              += 2;
                continue;
            } else return BAD_ARGUMENTS_ERROR_TEF;
        } else if(scmp(argv[position], "drows"))
        {
            if(position + 2 < argc)
            {
				if(!is_str_number(argv[position + 1], WHOLE) || !is_str_number(argv[position + 2], WHOLE))
					return BAD_ARGUMENTS_ERROR_TEF;
			   
			    tedit_t->d_row.params[tedit_t->d_row.param_c].x1 = atoi(argv[position + 1]);
			    tedit_t->d_row.params[tedit_t->d_row.param_c].x2 = atoi(argv[position + 2]);
				
			    if(tedit_t->d_row.params[tedit_t->d_row.param_c].x1 >
				   tedit_t->d_row.params[tedit_t->d_row.param_c].x2)
			    { return BAD_ARGUMENTS_ERROR_TEF; }
                   
				tedit_t->d_row.param_c++;
                tedit_t->arg_count_te += 3;
                position              += 3;
                continue;
            } else return BAD_ARGUMENTS_ERROR_TEF;
        } else if(scmp(argv[position], "acol"))
        {
            tedit_t->arg_count_te++;
            tedit_t->a_col.param_c++;
        } else if(scmp(argv[position], "icol"))
        {
            if(position + 1 < argc)
            {
                if(!is_str_number(argv[position + 1], WHOLE))
					return BAD_ARGUMENTS_ERROR_TEF;
				
				tedit_t->i_col.params[tedit_t->i_col.param_c].x1 = atoi(argv[position + 1]);
				tedit_t->i_col.params[tedit_t->i_col.param_c].x2 = tedit_t->i_col.params[tedit_t->i_col.param_c].x1;

				tedit_t->i_col.param_c++;
                tedit_t->arg_count_te += 2;
                position              += 2;
                continue;
            } else
                return BAD_ARGUMENTS_ERROR_TEF;
        } else if(scmp(argv[position], "arow"))
        {
            tedit_t->arg_count_te++;
            tedit_t->a_row.param_c++;
        } else if(scmp(argv[position], "irow"))
        {
            if(position + 1 < argc)
            {
                 if(!is_str_number(argv[position + 1], WHOLE))
					 return BAD_ARGUMENTS_ERROR_TEF;
							   
			    tedit_t->i_row.params[tedit_t->i_row.param_c].x1 = atoi(argv[position + 1]);
			    tedit_t->i_row.params[tedit_t->i_row.param_c].x2 = tedit_t->i_row.params[tedit_t->i_row.param_c].x1;

				tedit_t->i_row.param_c++;
			    tedit_t->arg_count_te += 2;
			    position              += 2;
				continue;
            } else return BAD_ARGUMENTS_ERROR_TEF;
        }
        position++;
    }
	
	if(tedit_t->d_col.param_c > 1) sort_del_reps(tedit_t->d_col.params, &(tedit_t->d_col.param_c));
	if(tedit_t->i_col.param_c > 1) sort_del_reps(tedit_t->i_col.params, &(tedit_t->i_col.param_c));
	if(tedit_t->d_row.param_c > 1) sort_del_reps(tedit_t->d_row.params, &(tedit_t->d_row.param_c));
	if(tedit_t->i_row.param_c > 1) sort_del_reps(tedit_t->i_row.params, &(tedit_t->i_row.param_c));
	
    return 0;
}

/**
 * Inserts an empty column after the last column in each row
 *
 * @return 0 if col has been added otherwise error code
 */
int acol_f(row_info *info)
{
    if(info->cache[info->l] == EOF)
        return 0;
    if(info->l + 1 < MAX_ROW_LENGTH)
    {
        info->cache[info->l] = info->seps.separators[0];
        info->cache[(++info->l)] = 10;

        info->row_seps.number_of_seps++;
    } else
        return MAX_LENGTH_REACHED;
    return 0;
}

/**
 * Inserts a new row after the last line
 */
void arow_f(row_info *info)
{
    if(info->cache[info->l] == EOF)
    {
        char temp = info->cache[info->l];
        for(int k = 0; k < info->row_seps.number_of_seps; k++)
            info->cache[info->l++] = info->seps.separators[0];

        info->cache[info->l] = 10;
        info->cache[++(info->l)] = temp;
    }
}

/**
 * Removes the column number R > 0
 *
 * @return 0 if success otherwise error code
 */
int dcol_f(int victim_column, row_info *info)
{
    if(info->cache[info->l] == EOF ||
	   victim_column > info->num_of_cols) // if user entered column greater than total number of column in the row do nothing
    { return 0; } // inspired by vim where user can enter 999dd if there are only two rows in the file

    int from      = (victim_column == 1) ? 0 : info->last_s[victim_column - 2];
    int to        = info->last_s[victim_column - 1];
    char flip_sep = 1;

    if(info->cache[to] == 10)
        to--;

    while(to >= from)
    {
        if((info->cache[to] == info->seps.separators[0]) && flip_sep)
        {
            flip_sep        = 0;
            info->cache[to] = DELETED_SEPARATOR;
            to--;
            continue;
        } else if((info->cache[to] == info->seps.separators[0]) && !flip_sep)
        {
            info->cache[to] = info->seps.separators[0];
            flip_sep        = 1;
            to--;
            continue;
        }
        if(info->cache[to] == DELETED_SEPARATOR)
        {
            to--;
            continue;
        }
        info->cache[to--] = 0;
    }

    if(!info->row_seps.number_of_seps)
        return 0;
	
    info->row_seps.number_of_seps--;
    return 0;
}

/**
 * Removes the row number R > 0
 *  If entered row greater than total number of rows in the file fust do nothing.
 *  In
 */
void drow_f(row_info *info)
{
	if(info->cache[info->l] == EOF)
		return;
	
	info->deleted_row = 1;
}

/**
 * Inserts the column before the column R > 0. Inserts char 3 and in print() changes it to separator
 *
 * @return 0 if success otherwise error code
 */
int icol_f(int victim_column, row_info *info,fun_option option )
{
    if(info->l + 1 >= MAX_ROW_LENGTH)
        return MAX_LENGTH_REACHED;
	
    if(info->cache[info->l] == EOF || victim_column > info->num_of_cols)
        return 0;
	
    int j = 0;
    int stop = (victim_column == 1) ? 0 : info->last_s[victim_column - 2];

    for(j = info->l + 1; j > stop; j--)
        info->cache[j] = info->cache[j - 1];

    // SEPARATOR_TO_PRINT
    // Program prevents problems with compatibility dcol and icol functions using this special character
	info->cache[j] = (option == CHECK_INSERTED_SEPS) ? SEPARATOR_TO_PRINT : info->seps.separators[0];
    info->row_seps.number_of_seps++;
    info->l++;
    return 0;
}

/**
 * Inserts the row before the row R > 0
 */
void irow_f(int victim_row, row_info *info)
{
    if(info->current_row == victim_row)
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
 *  (check for natural numbers, check that one argument is greater / greater than or equal to / less than another/ 0).
 *  Checka if parameter is a floating-pointer number.
 *
 *  Counts the number of parameters, which belong to functions,
 *  in order to compare it with the total number of command line arguments and return an error if theese numbers do not match
 *
 * @param is_dlm
 * 	  If it is equal to 1, it means that a line with separators has been entered and the parsing of arguments starts from the 3rd argument.
 *    Otherwise, the parsing of arguments starts from the 1st argument.
 * @return error code or 0 if success
 */
int dpf_init(int argc, char *argv[], data_processing *daproc, char is_dlm)
{
    //region variables
    int position = (is_dlm) ? 3 : 1;
    int first_arg = position;
    /**
     * By default it means processing only last row
     */
    daproc->sel.from       = 0;
    daproc->sel.to         = 0;
    int k                  = 0;

    /**
     * Dpf_option is data processing function option
     * Each function has its own option.
     * 0 means there is not dpf functino in arguments
     */
    daproc->dpf_option     = NO_DPF;
    /**
     * The same thing with row sel
     * 0 by default, but if there is row sel in arguments it becomes a number, defines the certain sel
     */
    daproc->sel.sel_option = NO_SELECTION;
    daproc->arg_count_dp   = 0;
    //endregion

    while(position < argc)
    {
        if(position == first_arg)
        {
            if(scmp(argv[position], "rows"))
            {
                if(position + 3 < argc)
                {
                    daproc->sel.from = atoi(argv[position + 1]);
                    daproc->sel.to = atoi(argv[position + 2]);

                    if(!daproc->sel.from)
                    {
                        if(argv[position + 1][0] != '-' || argv[position + 1][1])
                            return SELECTION_IOOR_ERROR;
                    }else if(!is_str_number(argv[position + 1], WHOLE))
						return NON_INT_ARGUMENTS_ERROR;
					
                    if(!daproc->sel.to)
                    {
                        if(argv[position + 2][0] != '-' || argv[position + 2][1])
                            return SELECTION_IOOR_ERROR;
                    }else if(!is_str_number(argv[position + 2], WHOLE))
						 return NON_INT_ARGUMENTS_ERROR;

                    if((!daproc->sel.from && daproc->sel.to) ||
					   (daproc->sel.from  && daproc->sel.to &&
                       (daproc->sel.from  > daproc->sel.to)))
                    {
                        return SELECTION_IOOR_ERROR;
                    }

                    daproc->sel.sel_option = ROWS;
                    daproc->arg_count_dp   += 3;
                    position               += 3;
                    continue;
                } else
                    return TOO_FEW_ARGS_AFTER_SELECTION;
            } else if(scmp(argv[position], "beginswith"))
            {
                daproc->sel.sel_option = BEGINSWITH;
            } else if(scmp(argv[position], "contains"))
            {
                daproc->sel.sel_option = CONTAINS;
            }

            if((daproc->sel.sel_option == BEGINSWITH) || (daproc->sel.sel_option == CONTAINS))
            {
                if(position + 3 < argc)
                {
                    if(!is_str_number(argv[position + 1], WHOLE))
                        return NON_INT_ARGUMENTS_ERROR;

                    daproc->sel.from = atoi(argv[position + 1]);

                    while(argv[position + 2][k])
                    {
                        if(k == CELL_LENGTH)
                            return SELECTION_TOO_LONG_PATTERN_ERROR;

                        daproc->sel.pattern[k] = argv[position + 2][k];
                        daproc->sel.pattern_len++;
                        k++;
                    }
                    daproc->arg_count_dp += 3;
                    position++;
                    continue;
                } else return WRONG_NUMBER_OF_ARGS_DP;
            }
        }
        if     (scmp(argv[position], "tolower"    )) daproc->dpf_option = TOLOWER;
        else if(scmp(argv[position], "toupper"    )) daproc->dpf_option = TOUPPER;
        else if(scmp(argv[position], "round"      )) daproc->dpf_option = ROUND;
        else if(scmp(argv[position], "int"        )) daproc->dpf_option = INT;

        if((daproc->dpf_option >= TOLOWER) && (daproc->dpf_option <= INT))
        {
            if(position + 1 == argc - 1)
            {
                if(!is_str_number(argv[position + 1], WHOLE))
                    return NON_INT_ARGUMENTS_ERROR;
				
                daproc->num1 = atoi(argv[position + 1]);
            } else return WRONG_NUMBER_OF_ARGS_DP;
			
            daproc->arg_count_dp += 2;
            break;
        }
        if     (scmp(argv[position], "cset"       )) daproc->dpf_option = CSET;
        else if(scmp(argv[position], "copy"       )) daproc->dpf_option = COPY;
        else if(scmp(argv[position], "swap"       )) daproc->dpf_option = SWAP;
        else if(scmp(argv[position], "move"       )) daproc->dpf_option = MOVE;
        else if(scmp(argv[position], "csum"       )) daproc->dpf_option = CSUM;
        else if(scmp(argv[position], "cavg"       )) daproc->dpf_option = CAVG;
        else if(scmp(argv[position], "cmin"       )) daproc->dpf_option = CMIN;
        else if(scmp(argv[position], "cmax"       )) daproc->dpf_option = CMAX;
        else if(scmp(argv[position], "cseq"       )) daproc->dpf_option = CSEQ;
        else if(scmp(argv[position], "rsum"       )) daproc->dpf_option = RSUM;
        else if(scmp(argv[position], "ravg"       )) daproc->dpf_option = RAVG;
        else if(scmp(argv[position], "rmin"       )) daproc->dpf_option = RMIN;
        else if(scmp(argv[position], "rmax"       )) daproc->dpf_option = RMAX;
        else if(scmp(argv[position], "rseq"       )) daproc->dpf_option = RSEQ;
		else if(scmp(argv[position], "rcount"     )) daproc->dpf_option = RCOUNT;
        else if(scmp(argv[position], "ccount"     )) daproc->dpf_option = CCOUNT;
        else if(scmp(argv[position], "concatenate")) daproc->dpf_option = CONCAT;

        if(daproc->dpf_option == CSET)
        {
            if(argc - 1 == position + 2)
            {
                if(!is_str_number(argv[position + 1], WHOLE))
                    return NON_INT_ARGUMENTS_ERROR;
				
                if(slen(argv[position + 2]) > CELL_LENGTH)
                    return DPF_TOO_LONG_PATTERN_ERROR;
				
				daproc->num1 = atoi(argv[position + 1]);
                strcpy(daproc->str, argv[position + 2]);
                daproc->arg_count_dp += 3;
                break;
            } else return WRONG_NUMBER_OF_ARGS_DP;
        } else if((daproc->dpf_option >= COPY) && (daproc->dpf_option <= SWAP))
        {
            if(argc - 1 == position + 2)
            {
                daproc->num2 = atoi(argv[position + 1]);
                daproc->num1 = atoi(argv[position + 2]);

                if(!is_str_number(argv[position + 1], WHOLE) ||
                   !is_str_number(argv[position + 2], WHOLE))
                { return NON_INT_ARGUMENTS_ERROR; }
				
                if(daproc->dpf_option == MOVE)
                { swap(&daproc->num1, &daproc->num2); }
				
				
                daproc->arg_count_dp += 3;
                break;
            } else return WRONG_NUMBER_OF_ARGS_DP;
        } else if((daproc->dpf_option >= CSUM) && (daproc->dpf_option < CSEQ))
        {
            if(argc - 1 == position + 3)
            {
                if(!is_str_number(argv[position + 1], WHOLE) ||
                   !is_str_number(argv[position + 2], WHOLE) ||
                   !is_str_number(argv[position + 3], WHOLE))
                {
                    return NON_INT_ARGUMENTS_ERROR;
                }
				daproc->num1 = atoi(argv[position + 1]);
			    daproc->num2 = atoi(argv[position + 2]);
			    daproc->num3 = atoi(argv[position + 3]);

                if((daproc->num2 > daproc->num3) ||
                   ((daproc->num1 >= daproc->num2) &&
                    (daproc->num1 <= daproc->num3)))
                {
                    return DPF_IOOR_ERROR;
                }
                daproc->arg_count_dp += 4;
                break;
            } else return WRONG_NUMBER_OF_ARGS_DP;
        } else if(daproc->dpf_option >= CSEQ && daproc->dpf_option <= RSEQ)
        {
            if(daproc->dpf_option == CSEQ)
            {
                if(argc - 1 != position + 3)
                    return WRONG_NUMBER_OF_ARGS_DP;
				
                daproc->arg_count_dp += 4;
            } else if(argc - 1 != position + 4)
                return WRONG_NUMBER_OF_ARGS_DP;

            if(!is_str_number(argv[position + 1], WHOLE) ||
               !is_str_number(argv[position + 2], WHOLE))
            {
                return NON_INT_ARGUMENTS_ERROR;
            }
			daproc->num1 = atoi(argv[position + 1]);
			daproc->num2 = atoi(argv[position + 2]);

            if(daproc->dpf_option == CSEQ)
            {
                if(daproc->num1 > daproc->num2)
                    return DPF_IOOR_ERROR;
				
				if(!is_str_number(argv[position + 3], RATIONAL))
					return NON_RAT_ARGUMENTS_ERROR;
				
                daproc->num4 = atof(argv[position + 3]);
            } else if(daproc->dpf_option == RSEQ)
            {
               
				daproc->num3 = atoi(argv[position + 3]);
				
                if((daproc->num3 == 0) && (argv[position + 3][0] == '-') && !argv[position + 3][1])
                {
                    daproc->num3 = 0;
                } else
                {
                    if(daproc->num2 > daproc->num3)
                        return DPF_IOOR_ERROR;
					if(!is_str_number(argv[position + 3], WHOLE))
						return NON_INT_ARGUMENTS_ERROR;
                }
				
				if(!is_str_number(argv[position + 4], RATIONAL))
					return NON_RAT_ARGUMENTS_ERROR;
				
                daproc->num4 = atof(argv[position + 4]);
                daproc->arg_count_dp += 5;
            }
            break;
        } else if(daproc->dpf_option >= RSUM && daproc->dpf_option <= RCOUNT)
        {
            if(argc - 1 != position + 3)
                return WRONG_NUMBER_OF_ARGS_DP;
			
            if(!is_str_number(argv[position + 1], WHOLE) ||
               !is_str_number(argv[position + 2], WHOLE) ||
               !is_str_number(argv[position + 3], WHOLE))
            { return NON_INT_ARGUMENTS_ERROR; }
			
			daproc->num1 = atoi(argv[position + 1]);
			daproc->num2 = atoi(argv[position + 2]);
			daproc->num3 = atoi(argv[position + 3]);

            if(daproc->num2 > daproc->num3)
                return DPF_IOOR_ERROR;
			
            daproc->arg_count_dp += 4;
            break;
        } else if(daproc->dpf_option == CONCAT)
        {
            if(argc - 1 >= position + 2) // case concatenate with empty string
            {
                // if concatenate nnumber 2 > number_of_columns concatenates the whole row
                 if(!is_str_number(argv[position + 1], WHOLE) ||
					!is_str_number(argv[position + 2], WHOLE))
				 {  return NON_INT_ARGUMENTS_ERROR;}
				
				daproc->num1 = atoi(argv[position + 1]);
				daproc->num2 = atoi(argv[position + 2]);
				
                if(daproc->num1 > daproc->num2)
                    return DPF_IOOR_ERROR;
				
				if(argc - 1 == position + 3 )
				{
					strcpy(daproc->str, argv[position + 3]);
					daproc->arg_count_dp += 4;
				} else if(argc - 1 == position + 2)
				{
					daproc->arg_count_dp += 3;
				}else return WRONG_NUMBER_OF_ARGS_DP;
				break;
			}
		}
		position++;
	}
    return 0;
}

/**
 *  The string in column num1 will be converted to lowercase.
 */
int tolower_f(row_info *info, data_processing *daproc)
{
    int from = (daproc->num1 == 1) ? 0 : info->last_s[daproc->num1 - 2];

    while(from < info->last_s[daproc->num1 - 1])
    {
        if(info->cache[from] >= 'A' && info->cache[from] <= 'Z')
            info->cache[from] += 32;
        from++;
    }
	return 0;
}

/**
 *  The string in column num1 will be converted to uppercase.
 */
int toupper_f(row_info *info, data_processing *daproc)
{
    int from = (daproc->num1 == 1) ? 0 : info->last_s[daproc->num1 - 2];

    while(from < info->last_s[daproc->num1 - 1])
    {
        if(info->cache[from] >= 'a' && info->cache[from] <= 'z')
            info->cache[from] -= 32;
        from++;
    }
	return 0;
}

/**
 *  The string daproc-str will be set in the cell in column num1 * @param info
 */
int cset_f(row_info *info, data_processing *daproc)
{
    //region variables
    int left_b     = (daproc->num1 == 1) ? 0 : info->last_s[daproc->num1 - 2] + 1;
    int right_b    = info->last_s[daproc->num1 - 1];
    int cell_len   = right_b - left_b;
    int len_topast = slen(daproc->str);
    int diff       = len_topast - cell_len;
	int exit_code  = 0;
    //endregion
    if(diff > 0)
    {
		
        for(int j = info->l; j >= right_b; j--)
            info->cache[j + diff] = info->cache[j];
		
		right_b += diff;
        info->l += diff;
		if(info->l >= MAX_ROW_LENGTH)
			return  MAX_LENGTH_REACHED;
        for(int j = left_b; j < right_b; j++)
            info->cache[j] = 0;
		
    } else
    {
        for(int j = left_b; j < right_b; j++)
            info->cache[j] = 0;
    }
	
	row_info_init(&exit_code, info, NO_OPTION); // change to CHECK_INSERTED_SEPS
    left_b = (daproc->num1 == 1) ? 0 : info->last_s[daproc->num1 - 2] + 1;
    right_b = info->last_s[daproc->num1 - 1];

    for(int j = left_b, k = 0; k < len_topast; k++, j++)
    {
		info->cache[j] = daproc->str[k];
	}
	
	return exit_code;
}

/**
 *  In column num1 rounds the number to an integer.
 */
int round_f(row_info *info, data_processing *daproc)
{
    //region variables
    char dot                     = 0, is_invalid = 0;
    int from                     = (daproc->num1 == 1) ? 0 : info->last_s[daproc->num1 - 2] + 1;
    int to                       = info->last_s[daproc->num1 - 1];
    int number                   = 0;
    char number_str[CELL_LENGTH] = {0};
    int temp_from                = from;
    //endregion

    if(from >= to)
        return 0;
	
    while(from < to && !is_invalid)
    {
        if(!isnumber(info->cache[from]))
        {
            if(info->cache[from] == '-' && from == temp_from)
            {
                number_str[number++] = info->cache[from++];
                continue;
            } else if(info->cache[from] == '.' && !dot)
            {
                dot++;
            } else
            {
                is_invalid = 1;
            }
        }
        number_str[number++] = info->cache[from++];
    }

    if(is_invalid)
        return 0; // Not an error_code code because program can convert to int only columns with numbers. Otherwise skip the column

    if(atof(number_str) - atoi(number_str) >= 0.5)
        sprintf(daproc->str, "%d", atoi(number_str) + 1);
	else
        sprintf(daproc->str, "%d", atoi(number_str));

    cset_f(info, daproc);
	
	return 0;
}

/**
 *  Removes the decimal part of the number in column num1.
 */
int int_f(row_info *info, data_processing *daproc)
{
    //region variables

    char dot                     = 0, is_invalid = 0;;
    int from                     = (daproc->num1 == 1) ? 0 : info->last_s[daproc->num1 - 2] + 1;
    int to                       = info->last_s[daproc->num1 - 1];
    int number                   = 0;
    char number_str[CELL_LENGTH] = {0};
    int temp_from                = from;
    //endregion

    if(from >= to)
        return 0;
	
    while((from < to) && !is_invalid)
    {
        if(!isnumber(info->cache[from]))
        {
            if(info->cache[from] == '-' && from == temp_from)
            {
                number_str[number++] = info->cache[from++];
                continue;
            } else if(info->cache[from] == '.' && !dot)
            {
                dot++;
            } else
            {
                is_invalid = 1;
            }
        }
        number_str[number++] = info->cache[from++];
    }

    if(is_invalid)
        return 0; // Not an error_code because program can convert to int only columns with numbers. Otherwise skip the column
	
    sprintf(daproc->str, "%d", atoi(number_str));
    cset_f(info, daproc);
	return 0;
}

/**
 *  Copies informaton from column1 to column2
 */
int copy_f(row_info *info, data_processing *daproc)
{
    int j    = 0;
    int from = (daproc->num2 != 1) ? info->last_s[daproc->num2 - 2] + 1 : 0;
    int to   = info->last_s[daproc->num2 - 1];

	clear_str(daproc->str);
	
    while(from < to)
        daproc->str[j++] = info->cache[from++];

    cset_f(info, daproc);
	return 0;
}

/**
 *  Swaps two columns
 *  Write the line in column 3 to a temporary array
 *  Copy column 1 to column 3.
 *  Copy a string from a temporary array to paste it into column 1
 *  Swap function because the cset function uses daprots-> num1 (where to insert).
 *  Paste the contents of column 3 into column 3.
 *
 *  Swap columns back
 */
int swap_f(row_info *info, data_processing *daproc)
{
    if(daproc->num1 == daproc->num2)
        return 0;
    //region variables
    int j                        = 0;
    int from                     = (daproc->num1 != 1) ? info->last_s[daproc->num1 - 2] + 1 : 0;
    int to                       = info->last_s[daproc->num1 - 1];
    char temp_str_1[CELL_LENGTH] = {0};
    //endregion
	while(from < to)
		temp_str_1[j++] = info->cache[from++];

	copy_f(info, daproc);
	clear_str(daproc->str);
	strcpy(daproc->str, temp_str_1);

	swap(&daproc->num1, &daproc->num2);
	cset_f(info, daproc);
	swap(&daproc->num1, &daproc->num2);
	return 0;
}

/**
 * Moves the daproc.column1 before the daproc.column2
 */
int move_f(row_info *info, data_processing *daproc)
{
    if(daproc->num2 == daproc->num1 + 1)
        return 0;
	
    //region variables
    int from      = (daproc->num1 == 1) ? 0 : info->last_s[daproc->num1 - 2] + 1;
    int to        = info->last_s[daproc->num1 - 1];
    int j         = 0;
	int exit_code = 0;
    //endregion
	
	clear_str(daproc->str);
    while(from < to)
        daproc->str[j++] = info->cache[from++];
	
    icol_f(daproc->num2, info, CHANGE_ALL_SEPS_TO_THE_FIRST_SEP_FROM_DELIM);
	row_info_init(&exit_code, info, CHECK_INSERTED_SEPS);
	swap(&daproc->num1, &daproc->num2);
    cset_f(info, daproc);
	swap(&daproc->num1, &daproc->num2);
	
	if(daproc->num1 > daproc->num2)
		dcol_f(daproc->num1+1, info);
	else
		dcol_f(daproc->num1, info);
	
	return exit_code;
}

/**
 *  A cell representing the sum of cell values on the same row from num2 to num3 inclusive
 *  will be stored in the cell in num1
 *  Note: if thereis a text in the column which is not a number, csum(as  rsum ad so) will just skip it.
 *   (num2 <= num3, num1 must not belong to the interval <num1; num2>).
 */
int csum_f(row_info *info, data_processing *daproc)
{
    info->arithmetic.sum = 0;

    // go through all cells in the row
    for(int col = daproc->num2; col <= daproc->num3; col++)
    {
        if(!number_validate_str(daproc->str, info, col))
            continue;
        info->arithmetic.sum += atof(daproc->str);
    }
    sprintf(daproc->str, "%g", info->arithmetic.sum);
    cset_f(info, daproc);
	
	return 0;
}

/**
 *  Similar to csum, but the resulting value represents the arithmetic mean of the values.
 */
int cavg_f(row_info *info, data_processing *daproc)
{
    //region variables
    info->arithmetic.sum = 0;
    int cols_with_nums   = 0;
    //edregion

    for(int col = daproc->num2; col <= daproc->num3; col++)// go through all cells in the row
    {
        if(!number_validate_str(daproc->str, info, col))
            continue;
        else
            cols_with_nums++;

        info->arithmetic.sum += atof(daproc->str);
    }
    if(!cols_with_nums)
        cols_with_nums++;

    sprintf(daproc->str, "%g", info->arithmetic.sum / cols_with_nums);
    cset_f(info, daproc);
	
	return 0;
}

/**
 *  Similar to csum, but the resulting value represents the smallest value found.
 */
int cmin_f(row_info *info, data_processing *daproc)
{
    float min            = 0;
    info->arithmetic.sum = 0;

    for(int col = daproc->num2; col <= daproc->num3; col++)
    {
        if(!number_validate_str(daproc->str, info, col))
            continue;

        if(col == daproc->num2)
        {
            min = atof(daproc->str);
            info->arithmetic.sum = min;
        } else
        {
            info->arithmetic.sum = atof(daproc->str);
            if(info->arithmetic.sum < min)
                min = info->arithmetic.sum;
        }
    }
    sprintf(daproc->str, "%g", min);
    cset_f(info, daproc);
	return 0;
}

/**
 *  Similar to csum, but the resulting value represents the arithmetic mean of the values.
 */
int cmax_f(row_info *info, data_processing *daproc)
{
    //region variables
    float max            = 0;
    info->arithmetic.sum = 0;
    //edregion

    for(int col = daproc->num2; col <= daproc->num3; col++)
    {
        if(!number_validate_str(daproc->str, info, col))
            continue;

        if(col == daproc->num2)
        {
            max = atof(daproc->str);
            info->arithmetic.sum = max;
        } else
        {
            info->arithmetic.sum = atof(daproc->str);
            if(info->arithmetic.sum > max)
                max = info->arithmetic.sum;
        }
    }
    sprintf(daproc->str, "%g", max);
    cset_f(info, daproc);
	return 0;
}

/**
 *  ccount column1 column2 column3 -
 *  The resulting value represents the number of non-empty values of the given cells.
 */
int ccount_f(row_info *info, data_processing *daproc)
{
    for(int j = daproc->num2; j <= daproc->num3; j++)
    {
        if(j == 1)
        {
            if(!info->last_s[j - 1])
                info->arithmetic.empties++;
        } else
        {
            if(info->last_s[j - 2] >= info->last_s[j - 1])
                info->arithmetic.empties++;
        }
    }
    sprintf(daproc->str, "%u", daproc->num3 - daproc->num2 + 1 - (info->arithmetic.empties));
    cset_f(info, daproc);
    clear_str(daproc->str);
	
	return 0;
}

/**
 *  Cseq column1 column2 column3 - inserts gradually increasing numbers (by one)
 *  starting with the value column3 into the cells in column1 to column2 inclusive
 */
int cseq_f(row_info *info, data_processing *daproc)
{
    info->arithmetic.sum = daproc->num4;
    int column1 = daproc->num1;

    for(int i = column1; i <= daproc->num2; i++, info->arithmetic.sum++)
    {
        daproc->num1 = i;
        sprintf(daproc->str, "%g", (float)info->arithmetic.sum);

        cset_f(info, daproc);

		clear_str(daproc->str);
    }
    daproc->num1 = column1;
	
	return 0;
}

/**
 *  Inserts the sum of cell values in num1 on rows num2 to num3 inclusive
 *  into the cell in num1 on row num3+1
 */
int rsum_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->num3 + 1)
    {
        sprintf(daproc->str, "%g", info->arithmetic.sum);
        cset_f(info, daproc);
        return 0;
    }
    char number_str[CELL_LENGTH] = {0};

    if(!number_validate_str(number_str, info, daproc->num1))
        return 0;

    info->arithmetic.sum += atof(number_str);
	
	return 0;
}

/**
 *  Calculates the arithmetic mean of numbers in columns num1 from row num2 to row num3 inclusive
 *  and inserts thsi value the cell in num1 on row num3 +1.
 */
int ravg_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->num3 + 1)
    {
        sprintf(daproc->str, "%g", info->arithmetic.sum / info->arithmetic.valid_row);
        cset_f(info, daproc);
        return 0;
    }
    char number_str[CELL_LENGTH] = {0};

    if(!number_validate_str(number_str, info, daproc->num1))
        return 0; // here is also 0. 1) Exsel does so. 2) It is logically and possible to count the arithmetic
	//mean in cells with numbers

    info->arithmetic.valid_row++;
    info->arithmetic.sum += atof(number_str);
	
	return 0;
}

/**
 *  Finds the minimum number in columns from row num2 to number inclusive
 *  into the cell in num1 on row num3 +1.
*/
int rmin_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->num3 + 1)
    {
        sprintf(daproc->str, "%g", info->arithmetic.min_max);
        cset_f(info, daproc);
        return 0;
    }

    char number_str[CELL_LENGTH] = {0};

    if(!number_validate_str(number_str, info, daproc->num1)) // 0 because it doesnt matter if there is
		// a column without number.
		{ return 0;}


    if(daproc->num2 == info->current_row)
    {
        info->arithmetic.min_max = atof(number_str);
        info->arithmetic.sum = info->arithmetic.min_max;
    }
    info->arithmetic.sum = atof(number_str);

    if(info->arithmetic.sum < info->arithmetic.min_max)
    {
		info->arithmetic.min_max = info->arithmetic.sum;
	}
	
	return 0;
}

/**
 *  Inserts the maximum of cell values in column num1 on rows num2 to num3 inclusive
 *  into the cell in num1 on row num3 +1.
*/
int rmax_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->num3 + 1)
    {
        sprintf(daproc->str, "%g", info->arithmetic.min_max);
        cset_f(info, daproc);
        return 0;
    }

    char number_str[CELL_LENGTH] = {0};

    if(!number_validate_str(number_str, info, daproc->num1)) // 0 because it doesnt matter if there is
		// a column without number.
		{ return 0;}

    if(daproc->num2 == info->current_row)
    {
        info->arithmetic.min_max = atof(number_str);
        info->arithmetic.sum = info->arithmetic.min_max;
    }
    info->arithmetic.sum = atof(number_str);

    if(info->arithmetic.sum > info->arithmetic.min_max)
    {
		info->arithmetic.min_max = info->arithmetic.sum;
	}
	
	return 0;
}

/**
 *  Inserts the number  of non-empty columns num1 from rows num2 to num3 inclusive
 *  into the cell in num1 on row num3 +1.
*/
int rconut_f(row_info *info, data_processing *daproc)
{
    if(info->current_row == daproc->num3 + 1)
    {
        sprintf(daproc->str, "%u",
			(daproc->num3 - daproc->num2 + 1) - info->arithmetic.empties);
		
        cset_f(info, daproc);
    }
    int from = (daproc->num1 == 1) ? 0 : info->last_s[daproc->num1 - 2] + 1;
    int to   = info->last_s[daproc->num1 - 1];

    if(from >= to)
    {
		info->arithmetic.empties++;
	}
	
	return 0;
}

/**
 *  Inserts ascending numbers in the cells of each row from the certain range.
 */
int rseq_f(row_info *info, data_processing *daproc)
{
    if(daproc->num3)
    {
        // range is from daroc->num2 to daproc->num3
        if(info->current_row >= daproc->num2 && info->current_row <= daproc->num3)
        {
            sprintf(daproc->str, "%g", daproc->num4);
            cset_f(info, daproc);
            daproc->num4++;
        }
    } else // if the user entered a hyphen
    {
        if(info->current_row >= daproc->num2)
        {
            sprintf(daproc->str, "%g", daproc->num4);
            cset_f(info, daproc);
            daproc->num4++;
        }
    }
	return 0;
}

/*
 *  Concatenates the strings of all cells in columns num1 to num2
 *  with the concatenation string STR, storing the result in column num1
 *  and deleting all other columns up to column num2.
 *  STR string can be empty.
 */
int concat_f(row_info *info, data_processing *daproc)
{
	int j         = 0, k = 0;
	int exit_code = 0;
    // if user entered column greater than number of columns it automatically concatenates all row
    int to = (daproc->num2 > info->num_of_cols) ? info->num_of_cols : daproc->num2;

    int len_topast = slen(daproc->str) - 1;
    if(info->l + (daproc->num1 - to) * (len_topast + 1) > CELL_LENGTH)
        return CONCAT_MAX_CELL_LEN_ERROR;

    for(j = to - 1; j >= daproc->num1; j--)
    {
        if(len_topast > 0)
        {
            for(k = info->l; k >= info->last_s[j - 1]; k--)
                info->cache[k + len_topast] = info->cache[k];
			
            info->l += len_topast;
        } else if(len_topast == -1) // if we user wants to concatenate cells with nothing
            len_topast++;

        for(k = info->last_s[j - 1]; k <= info->last_s[j - 1] + len_topast; k++)
        {    info->cache[k] = daproc->str[k - info->last_s[j - 1]];}
    }
    if(daproc->num1)
    {row_info_init(&exit_code, info, 0);}
	
	return exit_code;
}
//endregion

/**
 *  Determines what functions the user has entered using the switch statment .
 *
 *  Determines if the user has entered a row sel.
 *
 *  Checks if the parameters do not exceed the number of columns.
 *
 *  Đssigns a function pointer to the function that should be called.
 *
 *  Then calls the function
 */
int dpf_call(row_info *info, data_processing *daproc)
{
    if(info->cache[0] == EOF)
        return 0;
    //region Variables
	int k                    =  0,exit_code = 0, from = 0, to = 0;
    char string[CELL_LENGTH] = {0};
    int (*function_to_call)(row_info *info, data_processing *daproc);
    //endregion
    switch(daproc->dpf_option)
    {
        case TOLOWER:{ function_to_call = &tolower_f; break   ;}
        case TOUPPER:{ function_to_call = &toupper_f; break   ;}
        case ROUND  :{ function_to_call = &round_f  ; break   ;}
		case INT    :{ function_to_call = &int_f    ; break   ;}
		case COPY   :{ function_to_call = &copy_f   ; break   ;}
        case MOVE   :{ function_to_call = &move_f   ; break   ;}
        case SWAP   :{ function_to_call = &swap_f   ; break   ;}
        case CSET   :{ function_to_call = &cset_f   ; break   ;}
        case CSUM   :{ function_to_call = &csum_f   ; break   ;}
        case CAVG   :{ function_to_call = &cavg_f   ; break   ;}
        case CMIN   :{ function_to_call = &cmin_f   ; break   ;}
        case CMAX   :{ function_to_call = &cmax_f   ; break   ;}
        case CCOUNT :{ function_to_call = &ccount_f ; break   ;}
        case CSEQ   :{ function_to_call = &cseq_f   ; break   ;}
        case RSUM   :{ function_to_call = &rsum_f   ; break   ;}
        case RAVG   :{ function_to_call = &ravg_f   ; break   ;}
        case RMIN   :{ function_to_call = &rmin_f   ; break   ;}
        case RMAX   :{ function_to_call = &rmax_f   ; break   ;}
        case RCOUNT :{ function_to_call = &rconut_f ; break   ;}
        case RSEQ   :{ function_to_call = &rseq_f   ; break   ;}
        case CONCAT :{ function_to_call = &concat_f ; break   ;}
        case NO_DPF :{							      return 0;}
    }

    if(daproc->dpf_option >= TOLOWER && daproc->dpf_option <= INT)
    {
        //I moved the check of the suitability of the cell from here to the function,
        //thereby reducing the number of iterations and, possibly, the execution time of the program.
        //Now the check for the suitability of the cell(this means
        // there must be no text in the cell for arithmetic functions to be performed)
        //is in functions themselves.
        //The program has a code repetition, but it works faster due to fewer iterations
        if(daproc->num1 > info->num_of_cols)
            return DPF_IOOR_ERROR;
    } else if(daproc->dpf_option >= CSUM && daproc->dpf_option <= CCOUNT)
    {
        // --//--
        if(daproc->num3 > info->num_of_cols || daproc->num2 > info->num_of_cols)
            return DPF_IOOR_ERROR;
    } else if(daproc->dpf_option >= RSUM && daproc->dpf_option <= RCOUNT)
    {
        // --//--
        if(info->current_row < daproc->num2 ||// If we are out of range, do nothing
           info->current_row > daproc->num3 + 1 )
        {
            return 0;
        }
		if (info->num_of_cols < daproc->num1 || (info->is_lastrow && info->current_row < daproc->num3))
			return DPF_IOOR_ERROR;
		
    } else if(daproc->dpf_option >= COPY && daproc->dpf_option <= SWAP)
    {
        if(daproc->num1 == daproc->num2)
			return 0;
		
		 if(daproc->num1 > info->num_of_cols || daproc->num2 > info->num_of_cols)
			 return DPF_IOOR_ERROR;
    }

    switch(daproc->sel.sel_option)
    {
        case ROWS:
            if(!daproc->sel.from && !daproc->sel.to)
            {
                if(info->is_lastrow)
                    exit_code = function_to_call(info, daproc);

            } else if(daproc->sel.from && !daproc->sel.to)
            {
                if(info->current_row >= daproc->sel.from)
                    exit_code = function_to_call(info, daproc);

            } else if((info->current_row >= daproc->sel.from) && (info->current_row <= daproc->sel.to))
			{
				if(info->is_lastrow && (daproc->sel.to > info->current_row))
					return SELECTION_IOOR_ERROR;
				exit_code = function_to_call(info, daproc);
			}
			
            break;
        case BEGINSWITH:
            if(daproc->sel.from > info->num_of_cols)
                return SELECTION_IOOR_ERROR;

            from = (daproc->sel.from == 1) ? 0 : info->last_s[daproc->sel.from - 2] + 1;
            to   = info->last_s[daproc->sel.from - 1];
			
            if(from >= to)
                return 0;
			
			for(k = 0; k < daproc->sel.pattern_len; k++)
            {
                if(info->cache[from + k] != daproc->sel.pattern[k])
					return 0;
            }
            exit_code = function_to_call(info, daproc);
            break;
        case CONTAINS:
            from = (daproc->sel.from == 1) ? 0 : info->last_s[daproc->sel.from - 2] + 1;
            to   = info->last_s[daproc->sel.from - 1];

            if(from >= to)
                return 0;

            while(from < to)
                string[k++] = info->cache[from++];

            if(find(daproc->sel.pattern, string))
                exit_code = function_to_call(info, daproc);
            break;

        case NO_SELECTION:
            exit_code = function_to_call(info, daproc);
            return 0;
    }
	return exit_code;
}


/**
 *  Calls table editing functions that can return an error code.
 *  Functions are specifically called in this order so as not to conflict with each other.
 */
int tef_call(row_info *info, table_edit *tedit)
{
	int j         = 0;
    int exit_code = 0;
	
    for(j = tedit->d_col.param_c - 1; j >= 0; j--)
		for(int i = tedit->d_col.params[j].x2; i >= tedit->d_col.params[j].x1; i-- )
			if((exit_code = dcol_f(i, info)))
				return exit_code;
	
    for(j = 0; j < tedit->a_col.param_c; j++)
        if((exit_code = acol_f(info)))
            return exit_code;
	
    for(j = tedit->i_col.param_c - 1; j >= 0; j--)
		if((exit_code = icol_f(tedit->i_col.params[j].x1, info, CHECK_INSERTED_SEPS)))
            return exit_code;
	
	if(bin_search(&tedit->d_row, 0, tedit->d_row.param_c, info->current_row))
		drow_f(info);
    
	if(bin_search(&tedit->i_row, 0, tedit->i_row.param_c, info->current_row))
		irow_f(info->current_row, info);
	
	if(info->is_lastrow)
		for(j = 0; j < tedit->a_row.param_c; j++)
		   arow_f(info);
   
    return exit_code;
}

/**
 *  Parses command line arguments:
 *  Initializes delim,
 *  Initializes table editing functions and data processing functions.
 *  Either the table editing functions or the data processing function with row seectionl can be entered, otherwise error code is returned.
 *
 * @return error code or 0
 * 			error codes:
 * 		    TOO_FEW_ARGS_ERROR if no function names have been entered in the command line
 * 		    EXCESS_ARGUMENTS_ERROR if user entered extra arguments which the program did not expect
 * 		    error_code from tef_init()
 * 		    error_code from dpf_init()
 * 		    error_code from separators_init()
 */
int commandline_args_init(int argc, char **argv,row_info *info, table_edit *tedit, data_processing *daproc)
{
    int  exit_code       = 0;
    char is_dlm          = 0;
    int  tef_num_of_args = 0;
    int  dpf_num_of_args = 0;


    // Initializes array with separators. If user has not entered "-d" means we use ' ' as the separator
    if(argc > 2 && !scmp(argv[1], "-d"))
    {
        info->seps.number_of_seps = 1;
        info->seps.separators[0] = ' ';
    } else if(argc > 3 && scmp(argv[1], "-d"))
    {
        is_dlm = 1;
        exit_code = separators_init(argv[2], info);
        if(exit_code)
            return exit_code;
    } else if(argc == 1)
        return TOO_FEW_ARGS_ERROR;


    // Parses command line arguments. Compares argc and argumets belong to the functions(with function names).
    // If theese numbers are different, returns error code
    exit_code = tef_init(argc, argv, tedit, is_dlm);
    if(exit_code)
        return exit_code;
    tef_num_of_args = tedit->arg_count_te;
	
    if(!tef_num_of_args)
    {
        exit_code = dpf_init(argc, argv, daproc, is_dlm);
        if(exit_code)
            return exit_code;
        dpf_num_of_args = daproc->arg_count_dp;
		
        if(is_dlm) // it means there are 2 more arguments fot -d <delim>
        {
            if(dpf_num_of_args + 2 != argc - 1)
            {
                if(!dpf_num_of_args)
                    return TOO_FEW_ARGS_ERROR;
                return EXCESS_ARGUMENTS_ERROR;
            }
        } else
        {
            if(dpf_num_of_args     != argc - 1)
            {
                if(!dpf_num_of_args)
                    return TOO_FEW_ARGS_ERROR;
                return EXCESS_ARGUMENTS_ERROR;
            }
        }
    } else
    {
        if(is_dlm)
        {
            if(tef_num_of_args + 2 != argc - 1)
            {
                if(!tef_num_of_args)
                    return TOO_FEW_ARGS_ERROR;
                return EXCESS_ARGUMENTS_ERROR;
            }
        } else
        {
            if(tef_num_of_args     != argc - 1)
            {
                if(!tef_num_of_args)
                    return TOO_FEW_ARGS_ERROR;
                return EXCESS_ARGUMENTS_ERROR;
            }
        }
    }

    return 0;
}

void call_functions(int *exit_code, row_info *info, table_edit *tedit, data_processing *daproc)
{
    *exit_code = tef_call(info, tedit);
    *exit_code = dpf_call(info, daproc);
}


/**
 *
 *	Scans each line of stdin character by character.
 *	If the current line is not the first line, then temp_char is inserted into the 1st place of an erray representing current row (see below),
 *	If the scanned char is a \n or the EOF enters the body of the condition.
 *	If the character being scanned is a \n , it takes the next character and places it in temp_char.
 *	This is done in order to find out if the line is the last line of the file.
 *	Then calls the function for the line, passing the variable exit_code to it at the address in order to change it.
 *
 *	Next, the row is written to stdout(exit code can be changed in function print() if the error is occured or EOF is reached(retirns 1 ))
 *	Further, if exit_code is not 0, returns the exit_code.
 *
 * @param info       coontains informations about the line of the file
 * @param tedit
 * @param daproc
 * @return 1 if success otherwise error code
 */
int process_input(row_info *info, table_edit *tedit, data_processing *daproc)
{
    //region variables
	info->arithmetic.valid_row = 1;
	info->current_row          = 1;
	info->is_lastrow           = 0;
	info->arithmetic.sum       = 0;
	info->arithmetic.empties   = 0;
	info->l                    = 0;
	info->deleted_row          = 0;

    char first_symbol          = 0;
    /**
     * By defult it is 0, but functons can change this value. In this case program will be terminated
     */
    int exit_code             = 0;
    //endregion

   
    /**
     *  Scan the file line by line.
	 *  Checks if the line's length is less than max possible len
	 *  Checks if stdin is empty
	 *   Checks if the line is the last line of the file.
	 *   If program finds \n char call function, give line to the function to process it
     */
    do
    {
        if(!info->l && info->current_row > 1)
            info->cache[info->l] = first_symbol;
        else
            info->cache[info->l] = getchar();

        if((info->l == MAX_ROW_LENGTH - 1) && (info->cache[info->l] != 10))
            return MAX_LENGTH_REACHED;

        if(info->cache[info->l] == 10 || info->cache[info->l] == EOF)
        {
            if(info->cache[info->l] != EOF)
            {
                info->row_seps.number_of_seps = 0;

				row_info_init(&exit_code, info, CHANGE_ALL_SEPS_TO_THE_FIRST_SEP_FROM_DELIM);
                if(exit_code)
                    return MAX_CELL_LENGTH_ERROR;

                first_symbol = getchar();
				// the second condition is basicalluy for windows OS
				// if last line seems like aaa:AAAA:EOF
				if(first_symbol == EOF || info->cache[info->l] == EOF)
                    info->is_lastrow = 1;
            }
            if((info->l == 1) && (info->cache[info->l] == EOF))
                return EMPTY_STDIN_ERROR;

            call_functions(&exit_code, info, tedit, daproc);
            if(exit_code)
                return exit_code;

            print(&exit_code, info);
            if(exit_code)
                return exit_code;
			
            continue;
        }
        (info->l)++;
    } while(1);
	
	return 0; // this row can not be reached i guess
}

/**
 *  It first executes the functions by parsing command line arguments commandline_args_init.
 *  If an error occured, it returns an error and writes out an error code to the stderr
 *
 *  If no error occured, it calls process_input
 *   - // -
 *
 * @param exit_code Return value of the function process_input
 * @return 0 if success return code otherwise
 */
int return_function(int argc, char **argv)
{
    char *error_msg[] = {
            "ERROR: Empty stdin. There is no in input to edit it.",
            "ERROR: One of a row in the file is too long. Try to make in shorter.",
            "ERROR:  Separators you entered are not supported by the program to prevent undetermined behaviour.\n\t\t"
            "They mustn't be numbers or letters",
            "ERROR: Too much arguments. Program supports at most 100 arguments. Enter less arguments.",

            "ERROR:  Wrong parameters after table edit command.\n\t\tParameters after table edit command must be inteeger numbers"
            " greater than 0.\n\t\tIf there are 2 parameters 1-st must be less or equal than 2-nd",
            "ERROR: Too few arguments after row selection command. Enter more arguments.",
            "ERROR: Wrong number of arguments after data processing command. Enter valid number of arguments.",
            "ERROR: Index after data processing command is out of range.",
            "ERROR: Index after row selection is out of range.Enter positive number less than number of columns in table.",
            "ERROR: Pattern you entered after row selection parameter is longer than max row lenght",
            "ERROR: Non integer arguments. Arguments must be without dcimal part, they must not contain string "
            "characters. Enter valid arguments.",
            "ERROR: One of the cells is too long. Max supported cell length is 100",
            "ERROR: No function names have been entered in command line. Enter apropriate arguments(function name with row selection(optional)) and its arguments.",
            "ERROR: Extra command line arguments found. Enter valid command line arguments",
            "ERROR: Pattern you've entered ater data processing functinon is too long. Enter shorter pattern. ",
			"ERROR: Parameter you've entered after data processing function is not a number. Enter a number after the data processing function",
			"ERROR: Concatenated column is to long. Maximum supported length of cell is 100",
			"ERROR: The column in range of a data processing function is not a number when function requires it. Change a range on a column or use set functions to insert a number."
    };
	
	row_info        info;
    table_edit      tedit;
    data_processing daproc;
	int exit_code;
	
	// initialise delim string and functions user've entered in command line
	// return an error code if wrong parameters have been entered or no parameters have been entered
    exit_code = commandline_args_init(argc, argv, &info, &tedit, &daproc);
    if(exit_code > 1)
	{
		fprintf(stderr, "\nErrNo %d %s %s", exit_code, error_msg[exit_code - 2], "-h or -help for help\n");
		return exit_code;
	}
	
	// processing the file usong functions user've entered as aommand line parameters
	exit_code = process_input(&info, &tedit, &daproc);
    if(exit_code > 1)
    {
		fprintf(stderr, "\nErrNo %d %s %s", exit_code, error_msg[exit_code - 2], "-h or -help for help\n");
	}

	return (exit_code == 1) ? 0 : exit_code;
}

int print_documentation()
{
	
    printf("PROGRAM                                           Table Editor\n"
           "DESCRIPTION         The program implements the basic operations of spreadsheets.\n"
           "                    The input of the program is text data from stdin, the input of operations is \n"
           "                    through command line arguments and the program writes the result to stdout.\n"
		   "                    Max length of a cell is 100, max length of a row is 10KiB(10240 symbols)\n"
           "AUTHOR              Skuratovich Aliaksandr (c) 2020\n\n\n"
		   
           "EXECUTION SYNTAX\n"
           "             Table editing:      ./sheet [-d DELIM] [Commands for editing the table size]\n"
           "             Date processing:    ./sheet [-d DELIM] [Row selection] [Command for data processing]\n\n"
           "TABLE EDITING COMMANDS\n"
           "             irow R              - inserts a table row before row R > 0.\n"
           "             arow                - appends a new table row to the end of the table.\n"
           "             drow R              - deletes row number R > 0.\n"
           "             drows N M           - removes lines N to M (0 < N <= M). In the case of N = M,\n"
           "                                   the command behaves the same as drow N.\n"
           "             icol C              - inserts an empty column before the column given by the\n "
           "                                   number C > 0.\n"
           "             acol                - adds an empty column after the last column.\n"
           "             dcol C              - removes column number C > 0.\n"
           "             dcols N M           - removes columns N to M (0 < N <= M). In the case of N = M,\n"
		   "                                   the command behaves the same as dcol N.\n\n"
		   );
	printf(
           "DATA PROCESSING COMMANDS\n"
           "             cset C STR          - the string STR will be set in the cell in column C > 0.\n"
           "             tolower C           - the string in column C > 0 will be converted to lowercase.\n"
           "             toupper C           - the string in column C > 0 will be converted to uppercase.\n"
           "             round C             - in column C > 0 rounds a number to an integer.\n"
           "             int C               - removes the decimal part of the number in column C > 0.\n"
           "             copy N M            - overwrites the contents of cells in column M > 0\n"
           "                                   with the values ââfrom column N > 0.\n"
           "             swap N M            - swaps the values ââof cells in columns N > 0 and M > 0.\n"
           "             move N M            - moves column N > 0 before column M > 0.\n"
           "             csum C N M          - a number representing the sum of cell values ââon the same row\n"
           "                                   in columns N > 0 to M > 0 inclusive will be stored in the cell\n"
           "                                   in column C > 0 (N <= M, C must not belong to the\n"
           "                                   interval <N; M>).\n"
           "             cavg C N M          - similar to csum, but the resulting value represents the\n"
           "                                   arithmetic mean of the values.\n"
           "             cmin C N M          - similar to csum, but the resulting value represents the\n"
           "                                   smallest value found.\n"
           "             cmax C N M          - similar to cmin, but this is the maximum value found.\n"
           "             ccount C N M        - similar to csum, but the resulting value represents the number\n"
           "                                   of non-empty values ââof the given cells.\n"
           "             cseq N M B          - inserts gradually increasing numbers (by one) starting with \n"
           "                                   the value B into the cells in columns N > 0 to M > 0 inclusive\n"
           "                                   (N <= M, C must not belong to the interval <N; M>).\n"
           "             rseq C N M B        - in column C > 0, inserts ascending numbers starting with the\n"
           "                                   value B(can be floating-pointer number) in the cells of each row\n"
           "                                   from row N > 0 to row M > 0 inclusive. \n"
           "                                   The number M can be replaced by a '-'. In this case, it means\n"
           "                                   the last line of the file.\n"
           "             rsum C N M          - inserts the sum of cell values ââin column C on rows N to M\n"
           "                                   inclusive into the cell in column C on row M + 1.\n"
           "             ravg C N M          - similar to rsum, but the resulting value represents the \n"
           "                                   arithmetic mean.\n"
           "             rmin C N M          - similar to rsum, but the resulting value represents the \n"
           "                                   smallest value.\n"
           "             rmax C N M          - similar to rsum, but the resulting value represents the \n"
           "                                   largest value.\n"
           "             rcount C N M        - similar to rsum, but the resulting value represents the \n"
           "                                   number of non-empty values ââof the given cells.\n"
           "             concatenate N M STR - joins the strings of all cells in columns N to M by the\n"
           "                                   concatenate string STR, storing the result in column N \n"
           "                                   and deleting all other columns up to column M. \n"
		   "                                   The string STR can be empty.\n\n");
	printf(
           "ROW SELECTION COMMANDS\n"
           "             rows N M            - the processor will process only rows N to M inclusive (N <= M).\n"
           "                                   N = 1 means processing from the first line.\n"
           "                                   If the character '-' is entered instead of the number M,\n"
           "                                   it represents the last line of the input file.\n"
           "                                   If '-' is also replaced by an N column,\n"
           "                                   this means selecting only the last row.\n"
           "                                   If this command is not specified, all lines are\n"
           "                                   considered by default.\n"
           "             beginswith C STR    - the processor will process only those rows whose cell contents\n"
           "                                   in column C begin with the string STR.\n"
           "             contains C STR      - the processor will process only those rows whose cells in \n"
           "                                   column C contain the string STR.\n\n"

    );

    return 0;
}

int main(int argc, char *argv[])
{
    // print help message
    if(argc == 2 && (scmp(argv[1], "-h") || scmp(argv[1], "--help")))
        return print_documentation();
	
    // run the program
    return return_function(argc, argv);
}
