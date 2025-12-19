// Лабораторная 2. Арифметическое сжатие. Нехорошев Константин, гр.932423

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#define MAX 256
#define TOP_VALUE 65535
#define HALF (TOP_VALUE / 2 + 1)
#define FIRST_QTR (HALF / 2)
#define THIRD_QTR (FIRST_QTR * 3)

typedef struct {
    unsigned char Ch;
    unsigned int Frequency;
    unsigned int Low;
    unsigned int High;
} Symbol;

Symbol Symbols[MAX];
unsigned int Frequency_Table[MAX] = {0};
int Symbols_Count = 0;
unsigned int Total_Frequency = 0;
size_t Original_Size = 0;

unsigned char Out_Buffer = 0;
int Out_Bits = 0;

void Write_Bit(FILE *File_Output, int Bit); // Записывает один бит в выходной буфер, при заполнении байта сбрасывает в файл
void Flush_Bits(FILE *File_Output); // Дозаписывает оставшиеся биты в файл, дополняя нулями до полного байта
unsigned char In_Buffer = 0;
int In_Bits = 0;

int Read_Bit(FILE *File_Input); // Считывает один бит из входного файла, используя побайтовое чтение и буферизацию
void Build_Frequency_Table(); // Считывает файл text.txt, подсчитывает частоты всех байтов и строит кумулятивные диапазоны [Low, High) для каждого символа
void Write_Frequency_Table(FILE *File_Output); // Записывает в закодированный файл длину исходного текста, количество символов и для каждого его код и частоту
void Read_Frequency_Table(FILE *File_Input); // Считывает из закодированного файла длину текста, таблицу частот и восстанавливает кумулятивные диапазоны
void Encode_File(); // Кодирует содержимое файла text.txt с использованием арифметического алгоритма и сохраняет результат в encoded.txt
void Decode_File(); // Декодирует содержимое файла encoded.txt и сохраняет результат в decoded.txt
int Check_Files_Equal(const char *File1, const char *File2); // Сравнивает два файла по байтам

void Write_Bit(FILE *File_Output, int Bit) {
    Out_Buffer = (Out_Buffer << 1) | Bit;
    Out_Bits++;
    if (Out_Bits == 8) {
        fputc(Out_Buffer, File_Output);
        Out_Buffer = 0;
        Out_Bits = 0;
    }
}

void Flush_Bits(FILE *File_Output) {
    if (Out_Bits > 0) {
        Out_Buffer <<= (8 - Out_Bits);
        fputc(Out_Buffer, File_Output);
    }
}

int Read_Bit(FILE *File_Input) {
    if (In_Bits == 0) {
        int C = fgetc(File_Input);
        if (C == EOF) return 0;
        In_Buffer = (unsigned char)C;
        In_Bits = 8;
    }
    int Bit = (In_Buffer >> 7) & 1;
    In_Buffer <<= 1;
    In_Bits--;
    return Bit;
}

void Build_Frequency_Table() {
    FILE *File_Input = fopen("text.txt", "rb");
    if (!File_Input) {
        printf("Ошибка: Исходный файл text.txt не найден\n");
        exit(1);
    }
    int C;
    Original_Size = 0;
    while ((C = fgetc(File_Input)) != EOF) {
        Frequency_Table[C]++;
        Original_Size++;
    }
    fclose(File_Input);
    Symbols_Count = 0;
    for (int I = 0; I < MAX; I++) {
        if (Frequency_Table[I] > 0) {
            Symbols[Symbols_Count].Ch = (unsigned char)I;
            Symbols[Symbols_Count].Frequency = Frequency_Table[I];
            Symbols_Count++;
        }
    }
    unsigned int Cumulative = 0;
    for (int I = 0; I < Symbols_Count; I++) {
        Symbols[I].Low = Cumulative;
        Cumulative += Symbols[I].Frequency;
        Symbols[I].High = Cumulative;
    }
    Total_Frequency = Cumulative;
}

void Write_Frequency_Table(FILE *File_Output) {
    fwrite(&Original_Size, sizeof(Original_Size), 1, File_Output);
    fwrite(&Symbols_Count, sizeof(Symbols_Count), 1, File_Output);
    for (int I = 0; I < Symbols_Count; I++) {
        fputc(Symbols[I].Ch, File_Output);
        fwrite(&Symbols[I].Frequency, sizeof(unsigned int), 1, File_Output);
    }
}

void Read_Frequency_Table(FILE *File_Input) {
    fread(&Original_Size, sizeof(Original_Size), 1, File_Input);
    fread(&Symbols_Count, sizeof(Symbols_Count), 1, File_Input);
    Total_Frequency = 0;
    for (int I = 0; I < Symbols_Count; I++) {
        Symbols[I].Ch = fgetc(File_Input);
        fread(&Symbols[I].Frequency, sizeof(unsigned int), 1, File_Input);
        Symbols[I].Low = Total_Frequency;
        Total_Frequency += Symbols[I].Frequency;
        Symbols[I].High = Total_Frequency;
    }
}

void Encode_File() {
    FILE *File_Input = fopen("text.txt", "rb");
    FILE *File_Output = fopen("encoded.txt", "wb");
    if (!File_Input || !File_Output) {
        printf("Ошибка: Не удалось открыть файл\n");
        return;
    }
    Build_Frequency_Table();
    Write_Frequency_Table(File_Output);
    unsigned int Low = 0, High = TOP_VALUE;
    int Bits_To_Follow = 0;
    int C;
    while ((C = fgetc(File_Input)) != EOF) {
        int Index = 0;
        while (Symbols[Index].Ch != (unsigned char)C) Index++;
        unsigned int Range = High - Low + 1;
        High = Low + (Range * Symbols[Index].High) / Total_Frequency - 1;
        Low  = Low + (Range * Symbols[Index].Low)  / Total_Frequency;
        while (1) {
            if (High < HALF) {
                Write_Bit(File_Output, 0);
                while (Bits_To_Follow > 0) {
                    Write_Bit(File_Output, 1);
                    Bits_To_Follow--;
                }
            } 
            else if (Low >= HALF) {
                Write_Bit(File_Output, 1);
                while (Bits_To_Follow > 0) {
                    Write_Bit(File_Output, 0);
                    Bits_To_Follow--;
                }
                Low -= HALF;
                High -= HALF;
            } 
            else if (Low >= FIRST_QTR && High < THIRD_QTR) {
                Bits_To_Follow++;
                Low -= FIRST_QTR;
                High -= FIRST_QTR;
            } 
            else break;
            Low <<= 1;
            High = (High << 1) | 1;
        }
    }
    Bits_To_Follow++;
    if (Low < FIRST_QTR) {
        Write_Bit(File_Output, 0);
        while (Bits_To_Follow > 0) {
            Write_Bit(File_Output, 1);
            Bits_To_Follow--;
        }
    } 
    else {
        Write_Bit(File_Output, 1);
        while (Bits_To_Follow > 0) {
            Write_Bit(File_Output, 0);
            Bits_To_Follow--;
        }
    }
    Flush_Bits(File_Output);
    fclose(File_Input);
    fclose(File_Output);
}

void Decode_File() {
    FILE *File_Input = fopen("encoded.txt", "rb");
    FILE *File_Output = fopen("decoded.txt", "wb");
    if (!File_Input || !File_Output) {
        printf("Ошибка: Не удалось открыть файл\n");
        return;
    }
    Read_Frequency_Table(File_Input);
    unsigned int Low = 0, High = TOP_VALUE;
    unsigned int Value = 0;
    for (int I = 0; I < 16; I++) {
        Value = (Value << 1) | Read_Bit(File_Input);
    }
    for (size_t Count = 0; Count < Original_Size; Count++) {
        unsigned int Range = High - Low + 1;
        unsigned int Freq = ((Value - Low + 1) * Total_Frequency - 1) / Range;
        int Index = 0;
        while (Symbols[Index].High <= Freq) Index++;
        fputc(Symbols[Index].Ch, File_Output);
        High = Low + (Range * Symbols[Index].High) / Total_Frequency - 1;
        Low  = Low + (Range * Symbols[Index].Low)  / Total_Frequency;
        while (1) {
            if (High < HALF) {} 
            else if (Low >= HALF) {
                Value -= HALF;
                Low -= HALF;
                High -= HALF;
            } 
            else if (Low >= FIRST_QTR && High < THIRD_QTR) {
                Value -= FIRST_QTR;
                Low -= FIRST_QTR;
                High -= FIRST_QTR;
            } 
            else break;
            Low <<= 1;
            High = (High << 1) | 1;
            Value = (Value << 1) | Read_Bit(File_Input);
        }
    }
    fclose(File_Input);
    fclose(File_Output);
}

int Check_Files_Equal(const char *File1, const char *File2) {
    FILE *F1 = fopen(File1, "rb");
    FILE *F2 = fopen(File2, "rb");
    if (!F1 || !F2) {
        if (F1) fclose(F1);
        if (F2) fclose(F2);
        return 0;
    }
    int A, B;
    while (1) {
        A = fgetc(F1);
        B = fgetc(F2);
        if (A != B) {
            fclose(F1);
            fclose(F2);
            return 0;
        }
        if (A == EOF || B == EOF) break;
    }
    fclose(F1);
    fclose(F2);
    return 1;
}

int main() {
    setlocale(LC_ALL, "Rus");
    int Mode;
    printf("Лабораторная 2. Арифметическое сжатие:\n");
    printf("Выберите режим работы:\n");
    printf("1 — Кодирование\n");
    printf("2 — Декодирование\n");
    printf("> ");
    scanf("%d", &Mode);
    clock_t Start_Time = clock();
    if (Mode == 1) {
        Encode_File();
        printf("Кодирование выполнено. Файл с результатом encoded.txt создан\n");
        FILE *F = fopen("text.txt", "rb");
        if (F) {
            fseek(F, 0, SEEK_END);
            long Size1 = ftell(F);
            fclose(F);
            F = fopen("encoded.txt", "rb");
            fseek(F, 0, SEEK_END);
            long Size2 = ftell(F);
            fclose(F);
            printf("Степень сжатия: %.3f\n", (double)Size2 / (double)Size1);
        }
    }
    else if (Mode == 2) {
        Decode_File();
        printf("Декодирование выполнено. Файл с результатом decoded.txt создан\n");
        if (Check_Files_Equal("text.txt", "decoded.txt")) printf("Проверка: Исходный файл text.txt совпадает с декодированным результатом в файле decoded.txt\n");
        else printf("Проверка: Исходный файл text.txt не совпадает с декодированным результатом в файле decoded.txt\n");
    }
    else {
        printf("Ошибка: Выбран неверный режим\n");
        return 1;
    }
    double Time_Spent = (double)(clock() - Start_Time) / CLOCKS_PER_SEC;
    printf("Время работы: %.3f сек\n", Time_Spent);
    return 0;
}