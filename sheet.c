//region Includes
#include <stdio.h>
#include <math.h>
//#include <unistd.h>
#include <stdlib.h>
#include <string.h>
//endregion

//region Errors
#define EMPTY_STDIN_ERROR 101
#define LENGTH_ERROR 102
#define NUMBER_OF_ARGUMENTS_ERROR 103
#define BAD_ARGUMENTS_ERROR 104
//endregion

//region Lengths
#define MAX_ROW_LENGTH 1024
#define BUFFER_LENGTH 1024
#define CELL_LENGTH 100
//endregion

//region Sizes of arrays with functions
#define NULL_PARAM_SIZE 2
#define ONE_PARAM_SIZE 4
//endregion

//region Functions
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
 *
 * @param string1 String to compare
 * @param string2 String to compare
 * @return 1 whether two given strings are equal else 0
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
 * Moves bytes from one string to another. Number of bytes is min{strlen1, strlen1}
 * @param str_from to copy from
 * @param str_to to copy to
 */
void scpy(char *str_from, char *str_to)
{
    int str_from_len = slen(str_from);
    int str_to_len = slen(str_to);
    if(str_from_len <= str_to_len )
    {
        for(int position = 0; position < str_from_len; ++position)
        {
            str_to[position] = str_from[position];
        }
    }else
    {
        for(int position = 0; position < str_to_len; ++position)
        {
            str_to[position] = str_from[position];
        }
    }
}

/**
 * Prints first size+1 chars of string
 * @param string_to_print
 * @param size
 */
void print(char *string_to_print, int size)
{
    if(MAX_ROW_LENGTH+BUFFER_LENGTH< size){return;}
    else
    {
        for(int s = 0; s <= size;)
        {
            putchar(string_to_print[s]);
            s++;
        }
    }
}
/**
 * Chaecks if char of char is number.
 * @param suspect char to check if it is number
 * @return 1 if true otherwise 0
 */
char isnumber(char suspect)
{
    return (suspect >= 48 && suspect <= 57) ? 1 : 0;
}

/**
 * Transforms char to number
 * @param tonumber_c char to transform
 * @return number number from 0 to 9 or -1
 */
char c_tonum(char tonumber_c)
{
    return (isnumber(tonumber_c)) ? (tonumber_c-48) : -1;
}
/**
 * Shows if the char is , or .
 * @param suspect number to check
 * @return 1 if true otherwise false
 */
char iscomma_f(char suspect)
{
    return (suspect == ',' || suspect == '.') ? 1 : 0;
}

/**
 * Transforms string from  position "from" to position "to" to double
 *  to transform only last char -1 -1
 *  to transform from n to last char enter n -1
 *
 * @param str string to transform
 * @param from position from check
 * @param to position to check
 * @return double
 */
double s_todub(char* str, int from, int to )
{
    char power = 0;
    char minuspower = 0;
    char length = slen(str);
    double result = 0;
    char isnegative = 0;
    char iscomma = 0;
    char iter = 0;


    if(length <= to || length <= from  ){return 0;}

    if( to == -1 )
    {
        if(from == -1)
        {
            from = length-1;
            to = length;
        }else
        {
            to = length;
        }
        power = length - from;

    }else
    if(from == to)
    {
        power = 1;
        //to++;
    }
    else
    if(from < to)
    {
        power = to - from;
    }else return 0;


    int from_temp = from;
    while(from < to)
    {
        if(isnumber(str[from]))
        {
            if(iscomma == 1)
            {
                result += c_tonum(str[from]) * pow(10, --minuspower);
            }else
            {
                result += c_tonum(str[from]) * pow(10, --power);
            }
        }else
        if((iscomma == 0 && isnegative && (from_temp+1 < length) && iscomma_f(str[from_temp+1]))
           || (iscomma == 0 && iscomma_f(str[from])))
        {
            iscomma++;
        }
        if( str[from_temp] == '-' && iter == 0)
        {
            power--;
            isnegative++;
        }else
        if((!isnegative && iscomma == 1) || !isnumber(str[from]))
        {
            return 0;
        }if(iscomma > 1){return 0;}
        from++;
        iter++;
    }

    return (isnegative) ? result*-1 : result;
}

/**
 * Transforms string from  position "from" to position "to" to int
 *  to transform only last char -1 -1
 *  to transform from n to last char enter n -1
 *
 * @param str string to transform
 * @param from position from check
 * @param to position to check
 * @return int
 */
int s_toint(char* str, int from, int to )
{
    char power = 0;
    char length = slen(str);
    int result = 0;
    char isnegative = 0;
    char iter = 0;


    if(length <= to || length <= from  ){return 0;}

    if( to == -1 )
    {
        if(from == -1)
        {
            from = length-1;
            to = length;
        }else
        {
            to = length;
        }
        power = length - from;
    }else
    if(from == to)
    {
        power = 1;
        //to++;
    }
    else
    if(from < to)
    {
        power = to - from;
    }else return 0;

    int from_temp = from;
    while(from < to)
    {
        if(isnumber(str[from]))
        {
            result += c_tonum(str[from]) * pow(10, --power);
        }else
        if( str[from_temp] == '-' && iter == 0)
        {
            power--;
            isnegative++;
        }else
        {
            return 0;
        }
        from++;
        iter++;
    }

    return (isnegative) ? result*-1 : result;
}

//endregion


int current_row = 1;
int i = 0;
char cache[MAX_ROW_LENGTH+BUFFER_LENGTH];
//char cache_first_line[MAX_ROW_LENGTH];
const char seps_size = 127;
char separators[seps_size];
int number_of_separators = 0;
int separators_first_line = 0;


//region Separators
/**
 *
 * @param argv2 Delim string - string of entered separators
 * @return array with unique separators
 */
char* separators_init( char* argv2)
{
    char switcher;
    int j;
    int k = 0;
    int number_of_separators = 0;
    while(argv2[k] != '\0')
    {
        switcher = 0;
        j = k;
        while(argv2[++j] != '\0')
        {
            if(argv2[i] == argv2[j])
            {
                switcher++;
                break;
            }
        }
        if(switcher == 0){separators[number_of_separators++] = argv2[i];}
        k++;
    }
    return separators;
}
/**
 * Check if suspect is from the set of separators.
 * @param suspect char to check whether it is separator and replase by the main separator
 * @return 1 if is separator 0 otherwise
 */
int checksep(char suspect)
{
    for(int separator_position = 0; separator_position < slen(separators); separator_position++)
    {
        if(suspect == separators[separator_position])
        {
            suspect = separators[0];
            return 1;
        }
    }
    return 0;
}

/**
 *
 * @param row line with separators
 * @return number of separators
 */
int num_of_seps(char* row)
{
    int number = 0;

    for(int k = 0; row[k] != 10; )
    {
        if(checksep(row[k]))
        {
            number++;
        }
        k++;
    }
    return number;
}
//endregion

//region Table edit

//region Null parameters
/**
 * Inserts an empty column after the last column
 * @return 1 if col has been added 0 otherwise
 */
int acol_f()
{

    if(cache[i] == EOF ){ return 1; }

    if( i+1 <= MAX_ROW_LENGTH )
    {
        cache[i] = separators[0];
        cache[++i] = 10;
    }

    separators_first_line = num_of_seps(cache);
    return 0;
}
/**
 * Inserts a new row at the end of the file
 * @return 1 if success 0 otherwise
 */
int arow_f()
{

    if(cache[i] == EOF && (MAX_ROW_LENGTH+BUFFER_LENGTH > separators_first_line))
    {
        char temp = cache[i];
        for(int k = 0; k < separators_first_line; k++)
        {
            cache[i++] = separators[0];
        }
        cache[i] = 10;
        cache[++i] = temp;
        return 1;
    }
    return 0;
}
//endregion

//region One parameter

/**
 * Removes the column number R > 0
 * @param victim_column
 * @return
 */
int dcol_f(int victim_column)
{
    (void)victim_column;
    printf("dcol  ");

    return 1;
}

/**
 * Removes the row number R > 0
 * @param column
 * @return
 */
int drow_f(int victim_row)
{
    (void)victim_row;
    printf("drow  ");

    return 1;
}

/**
 * Inserts the column before the column R > 0
 * @param victim_column insert column before this nuber
 * @return
 */
int icol_f(int victim_column)
{
    (void)victim_column;
    return 1;
}

/**
 * Inserts the row before the row R > 0
 * @param victim_row insert row before this nuber
 * @return
 */
int irow_f(int victim_row)
{
    separators_first_line = num_of_seps(cache);

    //region deleteit
//    if(victim_row == 1 && current_row == 1 )
//    {
//        int j = 0;
//        for( j = i; j >= 0;)
//        {
//            cache[j+separators_first_line+1] = cache[j];
//            --j;
//        }
//        for(j = 0; j < separators_first_line; )
//        {
//            cache[j] = separators[0];
//            ++j;
//        }
//        cache[j] = 10;
//        i+=separators_first_line+1;
//        //j++;
//        //cache[i] = 10;
//        current_row++;
//    }
//    //printf("irow   ");
//endregion

    if(current_row+1 == victim_row)
    {
        i++;
        for(int j = 0; j < separators_first_line; )
        {
            cache[i] = separators[0];
            i++;
            j++;
        }
        cache[++i] = 10;

    }else
    if(current_row == 1 && victim_row == 1)
    {
        int j = 0;
        for( j = i; j >= 0;)
        {
            cache[j+separators_first_line+1] = cache[j];
            --j;
        }
        for(j = 0; j < separators_first_line; )
        {
            cache[j] = separators[0];
            ++j;
        }
        cache[j] = 10;
        i+=separators_first_line+1;
    }
    //current_row++;

    return 1;
}
//endregion
//endregion

//region Data processing
//endregion

/**
 * Parsing arguments and call functions
 * @return 0 if success
 */
char parse_arguments(int argc, char* argv[])
{
    char isfound = 0;
    /**
     * Initialize an array of separators
     */



    //region Arrays of functions, its pointers and names
    char* null_params[NULL_PARAM_SIZE] ={ "acol", "arow"};
    char* one_param[ONE_PARAM_SIZE] = { "dcol","drow","icol","irow"};

    int (*one_param_f[])(int) = {dcol_f,drow_f,icol_f,irow_f};
    int (*null_params_f[])() = {acol_f, arow_f};
    //endregion


    for(int j = 0; j < argc ; j++)
    {
        for(int k = 0; k < NULL_PARAM_SIZE; k++)
        {
            if(scmp(argv[j], null_params[k]))
            {
                isfound++;
                null_params_f[k]();
            }
        }
    }

    for(int j = 0; j < argc ; j++)
    {
        for(int k = 0; k < ONE_PARAM_SIZE; k++)
        {
            if(scmp(argv[j], one_param[k]) && argc > j+1 )
            {
                //if(s_toint(argv[j+1],0, -1) != 0)
                if(atoi(argv[j+1]) != 0)
                {
                    isfound++;
                    one_param_f[k](atoi(argv[j+1]));
                    //one_param_f[j](s_toint(argv[j+1], 0, -1));
                    j++;
                }else return BAD_ARGUMENTS_ERROR;
            }
        }
    }


    return (isfound == 0) ? BAD_ARGUMENTS_ERROR : 0 ;
}

/**
 *
 * @param argc Number of arguments of command line
 * @param argv Values arguments of command line
 * @return 0 if success else error
 */
int cache_init(int argc, char* argv[])
/*
 * The maximum supported length of a string in a cell or argument is 100. The maximum length of an entire row is 1024.
 * For longer strings, the program warns with an error message and terminates with an error code.
 * */
{

    if(argc > 3 && scmp(argv[1], "-d")){ separators_init(argv[2]); }
    else if(argc > 2 && !scmp(argv[1], "-d")) {separators[0] = ' ';}
    else return NUMBER_OF_ARGUMENTS_ERROR;


    /**
     * Scan the file line by line
     * if we find \n char go to the function parsing, parse functons, give line to the functions
     *
     */
    do
    {
        cache[i] = getchar();
        if (i == MAX_ROW_LENGTH-1 && cache[i] != 10){ return LENGTH_ERROR; }


        if(cache[i] == 10 || cache[i] == EOF)
        {
            if(current_row == 1)
            {
                separators_first_line = num_of_seps(cache);
            }

            parse_arguments(argc, argv);

            if(cache[i] == EOF)
            {
                print(cache, i-1);
                break;
            }

            print(cache, i);
            current_row++;
            i = 0;
            continue;
        }

        if(cache[i] == EOF)
        {
            break;
        }
        i++;

    }while(1);

    return 0;
}


int main(int argc, char* argv[])
{


//    //region keywords

//    char* two_params[] = {"dcols","drows","copy", "move", "swap" };
//    char* three_params[] = { "cavg", "ccount",  "cmax","cmin", "cseq","cset","csum",
//                             "int","ravg","rcount", "rmax","rmin","round","rseq","rsum","tolower","toupper" };
//    char* flags[] = {"-d", "rows", "beginswith", "contains"};
//    //endregion
//
//
//    int acol_f(), arow_f(), cavg_f(), count_f(), copy_f(), cmax_f(),cmin_f(), cseq_f(),cset_f(),csum_f(),
//            dcol_f(),dcols_f(),drow_f(),drows_f(),icol_f(),int_f(),irow_f(),move_f(),ravg_f(),rcount_f(),
//            rmax_f(),rmin_f(),round_f(),rseq_f(),rsum_f(),swap_f(),tolower_f(),toupper_f();
//
//    //region keyword_functions

//    int (*one_param_f [])(int) = { dcol_f, drow_f, icol_f, irow_f };
//    int (*two_params_f[])(int, int, char[]) = { dcols_f,drows_f,copy_f, move_f, swap_f };
//    //endregion


//
//
//    int (*keyword_functions[28]) (int, const char**, separators_struct) = {
//         acol_f, arow_f, cavg_f, count_f, copy_f, cmax_f, cmin_f, cseq_f, cset_f, csum_f,
//         dcol_f, dcols_f, drow_f, drows_f, icol_f, int_f, irow_f, move_f, ravg_f, rcount_f,
//         rmax_f, rmin_f, round_f, rseq_f, rsum_f, swap_f, tolower_f, toupper_f };
    if(argc >= 100){return BAD_ARGUMENTS_ERROR; }


    return cache_init(argc, argv);
}
