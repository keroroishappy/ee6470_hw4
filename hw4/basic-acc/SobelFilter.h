#ifndef SOBEL_FILTER_H_
#define SOBEL_FILTER_H_
#include <systemc>
#include <cmath>
#include <iomanip>
using namespace sc_core;

#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "filter_def.h"

struct SobelFilter : public sc_module {
  tlm_utils::simple_target_socket<SobelFilter> tsock;

  sc_fifo<unsigned char> i_r;
  sc_fifo<unsigned char> i_g;
  sc_fifo<unsigned char> i_b;
  sc_fifo<int> o_result;

  SC_HAS_PROCESS(SobelFilter);

  SobelFilter(sc_module_name n): 
    sc_module(n), 
    tsock("t_skt"), 
    base_offset(0) 
  {
    tsock.register_b_transport(this, &SobelFilter::blocking_transport);
    SC_THREAD(do_filter);
  }

  ~SobelFilter() {
	}

  unsigned int base_offset;
  int median[9];
  int median_bitmap[512][512];

  void do_filter(){
    for(int y = 0; y < 512; y++) {
      int median_count = 0;
      for(int x = 0; x < 512; x++) {
        if(x == 0){
          for(int z = 0;z < 9; z++){
            median[z] = (i_r.read() + i_g.read() + i_b.read())/3;
          }
          for(int i = 8; i > 0; i--){
            for(int j = 0; j < i-1; j++){
              if(median[j] > median[j+1]){
                int tmp = median[j];
                median[j] = median[j+1];
                median[j+1] = tmp;
              }
            }
          }  
          median_bitmap[y][x] = median[4];
          //cout << y << "," << x << "=" << median_bitmap[y][x];
        } else {
          for(int z = 0;z < 3; z++){
            median[median_count % 9] = (i_r.read() + i_g.read() + i_b.read())/3;
            median_count++;
          }
          for(int i = 8; i > 0; i--){
            for(int j = 0; j < i-1; j++){
              if(median[j] > median[j+1]){
                int tmp = median[j];
                median[j] = median[j+1];
                median[j+1] = tmp;
              }
            }
          }  
          median_bitmap[y][x] = median[4];
          //cout << y << "," << x << "=" << median_bitmap[y][x];
        }
        //cout << y << "," << x << "=" << median_bitmap[y][x];
      }
    }

    for (int y = 0; y < 512; y++) {
      for (int x = 0; x < 512; x++) {
        int sum = 0;
        for (int v = -1; v <= 1; v++) {
          for (int u = -1; u <= 1; u++) {
            int yy = y + v;
            int xx = x + u;
            if (yy >= 0 && yy < 512 && xx >= 0 && xx < 512) {
              sum += median_bitmap[yy][xx] * mask[v+1][u+1];
            } 
          }
        }
        int mean = (sum / 10);
        cout << "??????????  " << y << "," << x << "=" << mean << endl;
        o_result.write(mean);
      }
    }
  }

  void blocking_transport(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay){
    wait(delay);
    // unsigned char *mask_ptr = payload.get_byte_enable_ptr();
    // auto len = payload.get_data_length();
    tlm::tlm_command cmd = payload.get_command();
    sc_dt::uint64 addr = payload.get_address();
    unsigned char *data_ptr = payload.get_data_ptr();

    addr -= base_offset;


    // cout << (int)data_ptr[0] << endl;
    // cout << (int)data_ptr[1] << endl;
    // cout << (int)data_ptr[2] << endl;
    word buffer;

    switch (cmd) {
      case tlm::TLM_READ_COMMAND:
        // cout << "READ" << endl;
        switch (addr) {
          case SOBEL_FILTER_RESULT_ADDR:
            buffer.uint = o_result.read();
            break;
          default:
            std::cerr << "READ Error! SobelFilter::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
          }
        data_ptr[0] = buffer.uc[0];
        data_ptr[1] = buffer.uc[1];
        data_ptr[2] = buffer.uc[2];
        data_ptr[3] = buffer.uc[3];
        break;
      case tlm::TLM_WRITE_COMMAND:
        // cout << "WRITE" << endl;
        switch (addr) {
          case SOBEL_FILTER_R_ADDR:
            i_r.write(data_ptr[0]);
            i_g.write(data_ptr[1]);
            i_b.write(data_ptr[2]);
            break;
          default:
            std::cerr << "WRITE Error! SobelFilter::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
        }
        break;
      case tlm::TLM_IGNORE_COMMAND:
        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        return;
      default:
        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        return;
      }
      payload.set_response_status(tlm::TLM_OK_RESPONSE); // Always OK
  }
};
#endif
