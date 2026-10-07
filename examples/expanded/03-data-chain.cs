// 03. Цепочка зависимостей
// Цель: Несколько последовательных зависимых вычислений.
// В графе: Цепочка данных x -> doubled -> shifted -> result без слияний.
int ComputeChain(int x)
{
    int doubled = x * 2;
    int shifted = doubled + 3;
    int result = shifted - 1;
    return result;
}
