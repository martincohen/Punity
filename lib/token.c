typedef struct
{
    char *begin;
    char *end;
}
Token;

#define token_length(Token) ((Token)->end - (Token)->begin)

void
token_set_string(Token *token, const char *string, int string_length)
{
    if (string == 0) {
        string_length = 0;
    } else if (string_length == -1) {
        string_length = strlen(string);
    }

    token->begin = (char*)string;
    token->end = token->begin + string_length;
}

int
token_skip(Token *input, char s)
{
    while (input->begin < input->end && *input->begin == s) {
        input->begin++;
    }
    return input->begin != input->end;
}

int
token_delimited_by_char(Token *input, Token *token, char s)
{
    char *it = input->begin;
    while (it < input->end && *it != s) {
        it++;
    }
    if (it == input->end) {
        return 0;
    }

    if (token) {
        token->begin = input->begin;
        token->end = it;
    }

    input->begin = it + 1;
    return 1;
}

char *
string_push(Bank *bank, const char *string, int length)
{
    if (length < 0) {
        length = strlen(string);
    } else if (length == 0) {
        return bank->it;
    }

    char *s = bank_push(bank, length);
    memcpy(s, string, length);
    return s;
}

char *
token_push(Bank *bank, Token *token)
{
    return string_push(bank, token->begin, token_length(token));
}