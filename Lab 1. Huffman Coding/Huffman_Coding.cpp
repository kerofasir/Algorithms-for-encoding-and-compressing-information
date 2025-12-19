// Лабораторная 1. Код Хаффмана. Нехорошев Константин, гр.932423

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#define MAX 256

typedef struct Node {
    int Frequency;
    unsigned char C;
    struct Node *Left, *Right;
} Node;

typedef struct {
    unsigned char Bits[256];
    int Size;
} Code;

Code Table[MAX];
int Frequency_Table[MAX] = {0};

Node* Create_Node(unsigned char C, int Frequency, Node *Left, Node *Right); // Создает новый узел дерева Хаффмана из символа, его частоты и указателей на потомков
Node* Build_Tree(); // Строит дерево Хаффмана на основе таблицы частот символов
Node* Read_Frequency_Table(FILE *File_Input); // Считывает таблицу частот из файла и по ней заново строит дерево
void Free_Tree(Node *Root); // Рекурсивно освобождает память, выделенную под дерево Хаффмана
void Build_Table(Node *Root, unsigned char *Path, int Depth); // Рекурсивно обходит дерево и формирует коды Хаффмана для каждого символа
void Write_Frequency_Table(FILE *File_Output); // Записывает таблицу частот в файл перед записью закодированных данных
void Encode_File(Node *Root); // Кодирует содержимое файла text.txt и сохраняет результат в encoded.txt
void Decode_File(Node *Root); // Декодирует содержимое файла encoded.txt, используя дерево Хаффмана, и сохраняет результат в decoded.txt
int Check_Files_Equal(const char *File1, const char *File2); // Сравнивает два файла по байтам, чтобы убедиться, что декодированный текст совпадает с исходным

Node* Create_Node(unsigned char C, int Frequency, Node *Left, Node *Right) {
    Node *New_Node = (Node*)malloc(sizeof(Node));
    New_Node->C = C;
    New_Node->Frequency = Frequency;
    New_Node->Left = Left;
    New_Node->Right = Right;
    return New_Node;
}

Node* Build_Tree() {
    Node* Nodes[MAX];
    int Count = 0;
    for (int i = 0; i < MAX; i++) {
        if (Frequency_Table[i] > 0) Nodes[Count++] = Create_Node(i, Frequency_Table[i], NULL, NULL);
    }
    if (Count == 0) return NULL;
    while (Count > 1) {
        int Min1 = -1, Min2 = -1;
        for (int i = 0; i < Count; i++) {
            if (Min1 == -1 || Nodes[i]->Frequency < Nodes[Min1]->Frequency) Min1 = i;
        }
        for (int i = 0; i < Count; i++) {
            if (i == Min1) continue;
            if (Min2 == -1 || Nodes[i]->Frequency < Nodes[Min2]->Frequency) Min2 = i;
        }
        Node *Left = Nodes[Min1];
        Node *Right = Nodes[Min2];
        Node *Parent = Create_Node(0, Left->Frequency + Right->Frequency, Left, Right);
        Nodes[Min1] = Parent;
        Nodes[Min2] = Nodes[Count - 1];
        Count--;
    }
    return Nodes[0];
}

Node* Read_Frequency_Table(FILE *File_Input) {
    for (int i = 0; i < MAX; i++) {
        fread(&Frequency_Table[i], sizeof(int), 1, File_Input);
    }
    return Build_Tree();
}

void Free_Tree(Node *Root) {
    if (!Root) return;
    Free_Tree(Root->Left);
    Free_Tree(Root->Right);
    free(Root);
}

void Build_Table(Node *Root, unsigned char *Path, int Depth) {
    if (!Root->Left && !Root->Right) {
        Table[Root->C].Size = Depth;
        memcpy(Table[Root->C].Bits, Path, Depth);
        return;
    }
    if (Root->Left) {
        Path[Depth] = 0;
        Build_Table(Root->Left, Path, Depth + 1);
    }
    if (Root->Right) {
        Path[Depth] = 1;
        Build_Table(Root->Right, Path, Depth + 1);
    }
}

void Write_Frequency_Table(FILE *File_Output) {
    for (int i = 0; i < MAX; i++) {
        fwrite(&Frequency_Table[i], sizeof(int), 1, File_Output);
    }
}

void Encode_File(Node *Root) {
    FILE *File_Input = fopen("text.txt", "rb");
    FILE *File_Output = fopen("encoded.txt", "wb");
    if (!File_Input || !File_Output) {
        printf("Ошибка: Не удалось открыть файл\n");
        return;
    }
    Write_Frequency_Table(File_Output);
    unsigned char Byte = 0;
    int Bit_Count = 0;
    int C;
    while ((C = fgetc(File_Input)) != EOF) {
        Code Code = Table[C];
        for (int i = 0; i < Code.Size; i++) {
            Byte <<= 1;
            if (Code.Bits[i]) Byte |= 1;
            Bit_Count++;
            if (Bit_Count == 8) {
                fputc(Byte, File_Output);
                Byte = 0;
                Bit_Count = 0;
            }
        }
    }
    if (Bit_Count > 0) {
        Byte <<= (8 - Bit_Count);
        fputc(Byte, File_Output);
    }
    fclose(File_Input);
    fclose(File_Output);
}

void Decode_File(Node *Root) {
    FILE *File_Input = fopen("encoded.txt", "rb");
    FILE *File_Output = fopen("decoded.txt", "wb");
    if (!File_Input || !File_Output) {
        printf("Ошибка: Не удалось открыть файл\n");
        return;
    }
    Root = Read_Frequency_Table(File_Input);
    int C;
    unsigned char Byte;
    Node *P = Root;
    while ((C = fgetc(File_Input)) != EOF) {
        Byte = (unsigned char)C;
        for (int i = 7; i >= 0; i--) {
            int Bit = (Byte >> i) & 1;
            if (Bit) P = P->Right;
            else P = P->Left;
            if (!P->Left && !P->Right) {
                fputc(P->C, File_Output);
                P = Root;
            }
        }
    }
    fclose(File_Input);
    fclose(File_Output);
    Free_Tree(Root);
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
    printf("Лабораторная 1. Код Хаффмана:\n");
    printf("Выберите режим работы:\n");
    printf("1 — Кодирование\n");
    printf("2 — Декодирование\n");
    printf("> ");
    scanf("%d", &Mode);
    if (Mode == 1) {
        FILE *File_Input = fopen("text.txt", "rb");
        if (!File_Input) {
            printf("Ошибка: Исходный файл text.txt не найден\n");
            return 1;
        }
        int C;
        while ((C = fgetc(File_Input)) != EOF) {
            Frequency_Table[C]++;
        }
        fclose(File_Input);
        Node *Root = Build_Tree();
        if (!Root) {
            printf("Ошибка: Файл пустой\n");
            return 1;
        }
        unsigned char Path[256];
        Build_Table(Root, Path, 0);
        Encode_File(Root);
        Free_Tree(Root);
        printf("Кодирование выполнено. Файл с результатом encoded.txt создан\n");
    }
    else if (Mode == 2) {
        Decode_File(NULL);
        printf("Декодирование выполнено. Файл с результатом decoded.txt создан\n");
        if (Check_Files_Equal("text.txt", "decoded.txt")) printf("Проверка: Исходный файл text.txt совпадает с декодированным результатом в файле decoded.txt\n");
        else printf("Проверка: Исходный файл text.txt не совпадает с декодированным результатом в файле decoded.txt\n");
    }
    else printf("Ошибка: Выбран неверный режим\n");
    return 0;
}