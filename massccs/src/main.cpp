/*
 * This program is licensed granted by STATE UNIVERSITY OF CAMPINAS - UNICAMP ("University")
 * for use of MassCCS software ("the Software") through this website
 * https://github.com/cces-cepid/MassCCS (the "Website").
 *
 * By downloading the Software through the Website, you (the "License") are confirming that you agree
 * that your use of the Software is subject to the academic license terms.
 *
 * For more information about MassCCS please contact: 
 * skaf@unicamp.br (Munir S. Skaf)
 * guido@unicamp.br (Guido Araujo)
 * samuelcm@unicamp.br (Samuel Cajahuaringa)
 * danielzc@unicamp.br (Daniel L. Z. Caetano)
 * zanottol@unicamp.br (Leandro N. Zanotto)
 */

#include "headers/System.h"
#include <iostream>

int main(int argc, char *argv[]) {
  // check if the input is right
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " input.json" << std::endl;
    exit(1);
  }

  /**
   * Generate the system
   */
  System system(argv[1]);

  std::cout << "Program finished..." << std::endl;
  return 0;
}
