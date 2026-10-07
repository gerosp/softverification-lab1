// 05. Копия до изменения
// Цель: Копирование значения и последующее изменение исходной переменной.
// В графе: return читает right, связанный со старым определением left.
int CopyBeforeUpdate(int x)
{
    int left = x;
    int right = left;
    left = left + 1;
    return right;
}
