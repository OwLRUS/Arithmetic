#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

using namespace std;

#define code_value_bits 16

int indexOFsymbol(char symbol, vector<pair<char, unsigned int>> vec) 
{
    for (int i = 0; i < vec.size(); i++) 
    {
        if (symbol == vec[i].first) 
        {
            return i + 2;
        }
    }

    return -1;
}

void fput_bit(unsigned int bit, unsigned int* bit_len, unsigned char* file_bit, FILE* output)
{
    (*file_bit) = (*file_bit) >> 1;
    if (bit) (*file_bit) |= (1 << 7);

    (*bit_len)--;

    if ((*bit_len) == 0) 
    {
        fputc((*file_bit), output);
        (*bit_len) = 8;
    }
}

void bitsANDfollow(unsigned int bit, unsigned int* follow_bits, unsigned int* bit_len, unsigned char* write_bit, FILE* output_file)
{
    fput_bit(bit, bit_len, write_bit, output_file);

    for (; *follow_bits > 0; (*follow_bits)--) 
    {
        fput_bit(!bit, bit_len, write_bit, output_file);
    }
}

void ArCoder(const char* input_text = "text.txt", const char* output_text = "cipher.txt") 
{
    unsigned short* alfabet = new unsigned short[256];
    for (int i = 0; i < 256; i++) 
    {
        alfabet[i] = 0;
    }

    FILE* input_file = fopen(input_text, "rb");
    if (input_file == nullptr) 
    {
        puts("ArCoder ERROR: No such file or directory\n");
        exit(1);
    }

    unsigned char character = 0;

    while (!feof(input_file)) 
    {
        character = fgetc(input_file);
        if (!feof(input_file)) 
        {
            alfabet[character]++;
        }
    }

    fclose(input_file);

    vector<pair<char, unsigned int>> vec;
    
    for (int i = 0; i < 256; i++) 
    {
        if (alfabet[i] != 0) 
        {
            vec.push_back(make_pair(static_cast<char>(i), alfabet[i]));
        }
    }

    sort(vec.begin(), vec.end(), [](const pair<char, unsigned int>& left, const pair<char, unsigned int>& right) 
        {
                if (left.second != right.second) 
                {
                    return left.second >= right.second;
                }

                return left.first < right.first;
        });


    unsigned short* ranges = new unsigned short[vec.size() + 2];
    ranges[0] = 0;
    ranges[1] = 1;
    for (int i = 0; i < vec.size(); i++)
    {
        unsigned int b = vec[i].second;
        for (int j = 0; j < i; j++) 
        {
            b += vec[j].second;
        }
        ranges[i + 2] = b;
    }

    if (ranges[vec.size()] > (1 << ((code_value_bits - 2)) - 1))
    {
        puts("ArCoder ERROR: Symbols frequencies are too long");
        exit(1);
    }

    unsigned int low_value = 0;
    unsigned int high_value = ((static_cast<unsigned int>(1) << code_value_bits) - 1);

    unsigned int divider = ranges[vec.size() + 1];
    unsigned int diff = high_value - low_value + 1;
    unsigned int first_qtr = (high_value + 1) / 4;
    unsigned int half = first_qtr * 2;
    unsigned int third_qtr = first_qtr * 3;

    unsigned int follow_bits = 0;
    unsigned int bit_len = 8;
    unsigned char write_bit = 0;

    int j = 0;

    input_file = fopen(input_text, "rb");
    FILE* output_file = fopen(output_text, "wb +");

    char col_letters = vec.size();
    fputc(col_letters, output_file);

    for (int i = 0; i < 256; i++) 
    {
        if (alfabet[i] != 0) 
        {
            fputc(static_cast<char>(i), output_file);
            fwrite(reinterpret_cast<const char*>(&alfabet[i]),
                sizeof(unsigned short), 1, output_file);
        }
    }


    while (!feof(input_file)) 
    {  
        character = fgetc(input_file);

        if (!feof(input_file)) 
        {
            j = indexOFsymbol(character, vec);

            high_value = low_value + ranges[j] * diff / divider - 1;
            low_value = low_value + ranges[j - 1] * diff / divider;

            for (;;) 
            {
                if (high_value < half)
                {
                    bitsANDfollow(0, &follow_bits,
                        &bit_len, &write_bit, output_file);
                }
                else if (low_value >= half)
                {
                    bitsANDfollow(1, &follow_bits,
                        &bit_len, &write_bit, output_file);
                    low_value -= half;
                    high_value -= half;
                }
                else if ((low_value >= first_qtr) && (high_value < third_qtr))
                {
                    follow_bits++;
                    low_value -= first_qtr;
                    high_value -= first_qtr;
                }
                else 
                {
                    break;
                }

                low_value += low_value;
                high_value += high_value + 1;
            }
        }
        else 
        {
            high_value = low_value + ranges[1] * diff / divider - 1;
            low_value = low_value + ranges[0] * diff / divider;

            for (;;) 
            {
                if (high_value < half)
                {
                    bitsANDfollow(0, &follow_bits,
                        &bit_len, &write_bit, output_file);
                }
                else if (low_value >= half)
                {
                    bitsANDfollow(1, &follow_bits,
                        &bit_len, &write_bit, output_file);
                    low_value -= half;
                    high_value -= half;
                }
                else if ((low_value >= first_qtr) && (high_value < third_qtr))
                {
                    follow_bits++;
                    low_value -= first_qtr;
                    high_value -= first_qtr;
                }
                else 
                {
                    break;
                }

                low_value += low_value;
                high_value += high_value + 1;
            }

            follow_bits++;

            if (low_value < first_qtr)
            {
                bitsANDfollow(0, &follow_bits,
                    &bit_len, &write_bit, output_file);
            }
            else 
            {
                bitsANDfollow(1, &follow_bits,
                    &bit_len, &write_bit, output_file);
            }

            write_bit >>= bit_len;
            fputc(write_bit, output_file);
        }
        diff = high_value - low_value + 1;
    }

    fclose(input_file);
    fclose(output_file);
}

void compressValue(const char* input_text = "text.txt", const char* output_text = "cipher.txt")
{
    long long file_size = 0;
    long long compress_size = 0;

    struct stat s1 {};
    struct stat s2 {};

    if (!stat(input_text, &s1)) {
        file_size = s1.st_size;
    }
    else {
        perror("STAT ERROR ");
    }
    if (!stat(output_text, &s2)) {
        compress_size = s2.st_size;
    }
    else {
        perror("STAT ERROR ");
    }

    cout << "Compress value is: " << (compress_size + 0.0) / file_size << "\n";
}

int main() 
{
    ArCoder();
    compressValue();
}