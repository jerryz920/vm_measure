
probe(npage, high_bit, low_bit)
  page_set[low_bit]
  again:
    i <- rand() % low_bit;
    page <- get_random_page()
    if test_no_l2_miss(page_set[i], page)
      page_set[i] += page;
    if pages count in page_set < npage
      goto again or exit and report failure
  memory_walk(page_set)
