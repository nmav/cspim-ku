/* Just a benchmark program to test simulator speed. */

#define N 18

int count;

static void hanoi(int n, int from, int to, int extra)
{
	if(!n)
		return;
	hanoi(n-1, from, extra, to);
	++count;
	hanoi(n-1, extra, to, from);
}

int main(void)
{
	hanoi(N, 1, 2, 3);
	return 0;
}
