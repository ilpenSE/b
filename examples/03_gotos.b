main()
{
  extrn printf;

  if (1) goto label1;
  else goto label2;

label1:
  printf("label1");
label2: {
  printf("label2");
}
}
