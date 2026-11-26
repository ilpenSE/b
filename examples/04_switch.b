main()
{
  extrn printf;

  switch (a) {
  case 1: printf("case 1"); goto done;
  case 2: printf("case 2"); goto done;
  }
done: ;
}