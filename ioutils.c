
#include "ioutils.h"

/**
 * @brief Tokenises string allowing embbeded strings.
 * @details The behaviour is similar to strtok(), except a substring
 * nested within " " is considered a single token.
 * @param buffer The input string buffer, if buffer is NULL then the 
 * process continues.
 * @param token the ouput buffer for the next token.
 * @param size the size of the token buffer.
 * @returns the pointer to the unprocessed protion of the buffer, or
 * NULL if no buffer has been initialised or the end of buffer is reached.
 * @warning This code uses the null-termination character to determine the end
 * of the buffer. The behaviour is undefined if the buffer does not contain 
 * this character.
 */
const char * StringToken(const char * buffer, char* token,int size)
{
    char c;
    unsigned char state;
    int index;
    static const char *buf = NULL;
    static int pos = 0;
    
    if (buffer != NULL)
    {
        pos = 0;
        buf = buffer;
    }

    if (buf == NULL)
    {
        return NULL;
    }
    
    index = 0;
    state = S_TRIM;
    while (state != S_DONE && index < size)
    {
        c = buf[pos]; pos++;
        switch (state)
        {
            case S_TRIM: /*clearing leading whitespace*/
                switch(c)
                {
                    case ' ':
                    case '\n':
                    case '\t':
                        state = S_TRIM;
                        break;
                    case '"':
                        state = S_STRING;
                        break;
                    case '\0':
                        token[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        state = S_TOKEN;
                        token[index] = c;
                        index++;
                        break;
                }
                break;
            case S_TOKEN: // reading a token
                switch(c)
                {
                    case '"':
                        state = S_STRING;
                        break;
                    case ' ':
                    case '\t':
                    case '\n':
                    case '\0':
                        token[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        token[index] = c;
                        index++;
                        break;
                }
                break;
            case S_STRING: // reading a string
                switch (c)
                {
                    case '\0':
                    case '"':
                        token[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        token[index] = c;
                        index++;
                        break;
                }
                break;
        }
    }

    return  (c == '\0') ? NULL : buf + pos;
}

/**
 * @brief reads the next token from a file stream.
 * @details A token is a contiguous sequence of non-whitespace
 * characters, or a sequance bounded by double "s. 
 * Comment lines (identified by the #) are also ignored.
 * @param fp the input file stream
 * @param buffer the token output stream
 * @param size the bytes in buffer
 * @retVal 0 The State machine terminated at an accepting state, 
 * and token stores a valid token.
 * @retVal 1 The state machine terminated in a non-accepting state.
 */
int NextToken(FILE *fp,char * buffer, int size)
{
    char c;
    unsigned char state;
    int index;
    index = 0;
    state = S_TRIM;
    while (state != S_EOF && state != S_DONE && index < size)
    {
        c = fgetc(fp);
        switch (state)
        {
            case S_TRIM: /*clearing leading whitespace*/
                switch(c)
                {
                    case ' ':
                    case '\n':
                    case '\t':
                        state = S_TRIM;
                        break;
                    case '#':
                        state = S_COMMENT;
                        break;
                    case EOF:
                        state = S_EOF;
                        break;
                    case '"':
                        state = S_STRING;
                        break;
                    default:
                        state = S_TOKEN;
                        buffer[index] = c;
                        index++;
                        break;
                }
                break;
            case S_COMMENT: /*ignoring comment lines*/
                switch(c)
                {
                    case '\n':
                        state = S_TRIM;
                        break;
                    case EOF:
                        state = S_EOF;
                        break;
                    default:
                        state = S_COMMENT;
                        break;
                }
                break;
            case S_TOKEN:
                switch(c)
                {
                    case '"':
                        state = S_STRING;
                        break;
                    case ' ':
                    case '\t':
                    case '\n':
                    case EOF:
                        buffer[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        buffer[index] = c;
                        index++;
                        break;
                }
                break;
            case S_STRING:
                switch (c)
                {
                    case '"':
                    case EOF:
                        buffer[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        buffer[index] = c;
                        index++;
                        break;
                }
                break;
        }
    }
    return (state == S_EOF);
}

/**
 * @brief Reads the next line in the input file stream.
 * @details Just keeps reading until \n or EOF or read 
 * extactly size chars.
 * @param fp the input file stream
 * @param buffer the output line buffer
 * @param size the size of buffer
 * @return 1 if EOF is reached
 */
int ReadLine(FILE *fp,char *buffer,int size)
{
    char c;
    int index;
    index = 0;
    c = fgetc(fp);
    while (c != '\n' && c != EOF && index < size)
    {
        buffer[index] = c;
        index++;
        c = fgetc(fp);
    }
    buffer[index] = '\0';
    return (c == EOF);
}
