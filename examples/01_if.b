/* If-else test case for compiler */

main()
{
  extrn printf;
  if (1) printf("true");
  else printf("false");

  if (1) printf("if");
  else if (2) printf("else if");
  else printf("else");

  if (1) {}
  if (1) {} else {}
  if (1) ;
  if (1) ; else ;
}