/**
 * @file RFM_69.c
 * @desc driver for the RFM_69 radio transciever which is controlled using SPI by a SAMD21
 * @author Arsalan Syed
 * @date 2019-01-02
 * Last Author:
 * Last Edited On:
 */

#include <RFM_69.h>
#include <RFM69registers.h>
#include <sercom-spi.c>

/*variables for SPI*/



/*variables for radio transmission and recieving*/
#define volatile uint8_t RFM_69_BAUDRATE 
#define volatile uint8_t NETWORK_ID
#define volatile uint8_t NODEID
#define volatile uint8_t MESSAGECBT
#define volatile uint64_t SYNCWORD

/*variables for operation*/
define volatile uint8_t currentMode
define volatile uint8_t interrupts

 

void RFM_69_init(struct RFM_69_desc_t *descriptor,
                   struct sercom_spi_desc_t *spi_inst, uint32_t poll_period,
                   uint32_t cs_pin_mask, uint8_t cs_pin_group, uint8_t opMode)
	{

	/*ISSUE: NEED TO SET UP DESCRIPTOR*/

	 
	/*start in standby mode*/
	Write_Reg(REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY);

	/* set to packet mode, with Data modulation done through frequency shift keying (FSK), with no shaping */
    Write_Reg(REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00);

	/*set to high power mode. This is mandatory*/
	Write_Reg(REG_OCP, _isRFM69HW ? RF_OCP_OFF : RF_OCP_ON); //turns on the overload current protection 
	Write_Reg(REG_PALEVEL, (uint8_t)(RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | 0x1f)); //turns on power ampplifiers 1 and 2, and 0x1f sets their power to maximum

	/* setting the carrier frequency. For rocket applications, should be set to 915MHz*/
	if(freqBand == RF69_315MHZ){
	 	 Write_Reg(REG_FRFMSB, (uint8_t)RF69_315MHZ);
		 Write_Reg(REG_FRFMID, (uint8_t)RF69_315MHZ);
		 Write_Reg(REG_FRFLSB, (uint8_t)RF69_315MHZ);
	}
	if(freqBand == RF_FRFMID_433){
	 	 Write_Reg(REG_FRFMSB, (uint8_t)RF_FRFMID_433);
		 Write_Reg(REG_FRFMID, (uint8_t)RF_FRFMID_433);
		 Write_Reg(REG_FRFLSB, (uint8_t)RF_FRFMID_433);
	}
	 
	if(freqBand == RF_FRFMSB_915){
	 	 Write_Reg(REG_FRFMSB, (uint8_t)RF_FRFMSB_915);
		 Write_Reg(REG_FRFMID, (uint8_t)RF_FRFMSB_915);
		 Write_Reg(REG_FRFLSB, (uint8_t)RF_FRFMSB_915);

	/*set bitrate to 4.8 KBPs, check page 22 on the data sheet for other bitrates */
	Write_Reg(REG_BITRATEMSB, RF_BITRATEMSB_55555);
    Write_Reg(REG_BITRATELSB, RF_BITRATELSB_55555);

	/* setting frequency deviation to 5kH (default value)*/
	Write_Reg(REG_FDEVMSB, RF_FDEVMSB_50000);
	Write_Reg(REG_FDEVLSB, RF_FDEVLSB_50000);

	 /*sets the recieving bandwidth to the default value = 10kHz. Note: bitRate < 2 * bandwidth */
	 Write_Reg(REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2);

	 /*clears flags and FIFO*/
     Write_Reg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);

	 /*threshold of signal strength; when we should start listening*/
     Write_Reg(REG_RSSITHRESH, 220);

    /*start filling the FIFO if 1)sync interrupt occurs, 2)there are no errors in the sync word.  Also, we expect the 
	sync word to be 3 bytes long*/
    Write_Reg(REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0);

	/*setting the sync Word and network ID and nodeID*/
    Write_Reg(REG_SYNCVALUE1, SYNC_WORD);
    Write_Reg(REG_SYNCVALUE2, NETWORK_ID);
	Write_Reg(REG_NODEADRS, NODE);

	/*1)allow address filter 2)set to variable length packet mode 3)turn off encryption 4) turn on checksum 5)if crc fails, clear FIFO and do not generate payloadReady interrupt*/
    Write_Reg(REG_PACKETCONFIG1, RF_PACKET1_ADRSFILTERING_NODE| RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_ON | RF_PACKET1_CRCAUTOCLEAR_ON);

	/*1)set delay between each transmission to be 2bit (based on PA ramp down time - do not change), 2) clears FIFO after a packet has been delivered and FIFO read
	  3) disable AES encryption*/
	Write_Reg(REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF);

	/*maximum length of payload that can be recieved in variable length packet mode*/
    Write_Reg(REG_PAYLOADLENGTH, 66);
   
	/*when Fifo is not empty, and module is set to TX mode, the module will begin transmitting immedietly*/
    Write_Reg(REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE);

	/*default config: run DAGC continously in RX mode for fade margain improvement*/
    Write_Reg(REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0);
   
}

void RFM_69_service(){
	/*ISSUE: need to be able to handle more than 66 bytes*/


	/*checks the "payloadReady flag*/
	if((currentMode == RF_OPMODE_RECEIVER)&&(Read_Reg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)){
		

		/*must be in standby mode to read FIFO*/
		SetMode(RF69_MODE_STANDBY);

		/*wait until in standby mode*/
		while(Read_Reg(REG_IRQFLAGS1) != 0x80){
			//IRQ_FLAGS1 == 0x80 means MODEREADY is true
		}

		/*first byte in the FIFO specifies the length of the message*/
		uint8_t payloadLength = Read_Reg(REG_FIFO);
		
		if(payloadLength > 66){
			uint8_t payloadLength = 66;
			/*need to figure out how to handle larger packets, but 66 bytes should be plenty for our purposes */
		}

		/*FIFO currently: address byte, network byte, node byte DATA (what we want) */
		dataLength = payloadLength - 3;
		uint8_t data[dataLength + 2]; /*extra byte for RSSI reading and extra byte for null value (end of data)*/
		for(uint8_t i = 0; i<dataLength; i++){
			data[i] = Read_Reg(REG_FIFO);

		}

		/* addding RSSI value and then capping with a null to indicate end of array*/
		data[dataLength] = MeasureRSSI();
		data[dataLength + 1] = 0;

		/*go back to listening. No need to manually clear the FIFO, it's done for us by the hardware*/
		SetMode(RF69_MODE_RX);

		
	}

	

}

uint8_t MeasureRSSI(){

	/*initiate an RSSI reading*/
	write_Reg(REG_RSSICONFIG, ( REG_RSSICONFIG | RF_RSSI_START)) 
	
	/*wait until RSSI measurement is ready*/
	while(Read_Reg(REG_RSSICONFIG) & RF_RSSI_DONE != 0x4);

	return Read_Reg(REG_RSSIVALUE);
}


void Write_Reg(uint8_t *address, uint8_t value){
	/*ISSUE: NEED TO SET UP SPI SETTINGS*/


	//setting the r/w bit to write
	address = (address || 0x80)
	
	//the address will be null when we are trying to write more than one byte to the FIFO
	if(*address != NULL){
		//sending the address including r/w bit 
		sercom_spi_start(RFM69_desc_t.spi_inst,
                                *trans_id, RFM_69_baudrate,
                                cs_pin_group, cs_pin_mask,
                                &Address,8,
                                NULL, 0)
	}
	
	//send the payload
	if(value != NULL){
		sercom_spi_start(*spi_inst, *trans_id, RFM_69_baudrate,
                                cs_pin_group, cs_pin_mask,
                                &value, 66,
                                NULL,0)
		}
	
	//checks to see if the transaction is done 
	if (sercom_spi_transaction_done(struct sercom_spi_desc_t *spi_inst,  uint8_t trans_id) == 1 ){
		//set nss as high 
		//enable interupts 
		sercom_spi_clear_transaction(struct sercom_spi_desc_t *spi_inst, uint8_t trans_id)


	}


uint8_t Read_Reg(uint8_t address){
	/*ISSUE: NEED TO SET UP SPI SETTINGS*/

	
	uint8_t address = (address & 0x7F);
	uint8_t inBuffer, *inBufferPointer;

	if(
	sercom_spi_start(struct *spi_inst, *trans_id, RFM_69_baudrate, cs_pin_group, cs_pin_mask, NULL, 0, inBufferPointer, 1) != 0)
	{
	   /*there has been an error in transmission*/
	   exit(1);
	}
	

	//checks to see if the transaction is done 
	if (sercom_spi_transaction_done(struct sercom_spi_desc_t *spi_inst,  uint8_t trans_id) == 1 )
		 sercom_spi_clear_transaction(struct sercom_spi_desc_t *spi_inst, uint8_t trans_id);
		 return(inbuffer);


	}


								 
uint32_t getFrequency(){
	return RF69_FSTEP * (((uint32_t) Read_Reg(REG_FRFMSB) << 16) + ((uint16_t) Read_Reg(REG_FRFMID) << 8) + Read_Reg(REG_FRFLSB));
}

void SetFrequency(uint32_t desired_freqHz){
	
	//set to standby mode
	uint8 pre_configuration_mode = currentMode;
	if(currentmode != RF_OPMODE_STANDBY){
		currentMode = RF_OPMODE_STANDBY;
	}

	freqHz /= RF69_FSTEP //note: FSTEP is the minimum incriment between frequencies
	
	//note: frf = FSTEP * frf(23;0)

	Write_Reg(REG_FRFMSB, freqHz >> 16);  //writing to most significant byte 
    Write_Reg(REG_FRFMID, freqHz >> 8);   //writing to mid significant byte
    Write_Reg(REG_FRFLSB, freqHz);        //writing to least significant byte

	//return to the mode it was in before reconfiguration
	currentMode = pre_configuration_mode;
}

void SetMode(uint8_t opMode){
	
	switch(opMode){
		case RF69_MODE_TX:

			//only bits 4-2 in REG_OPMODE control the mode of the module. The rest are left alone
			Write_Reg(REG_OPMODE, Read_Reg(REG_OPMODE) & 0xE3 | RF_OPMODE_TRANSMITTER);
			currentMode = RF69_MODE_TX;
			break;

		case RF69_MODE_RX:
			Write_Reg(REG_OPMODE, (Read_Reg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
			currentMode = RF_OPMODE_RECEIVER;
			break;

		case RF69_MODE_SLEEP:
			Write_Reg(REG_OPMODE, (Read_Reg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
			currentMode = RF_OPMODE_SLEEP;
			break;

		case RF69_MODE_STANDBY:
			Write_Reg(REG_OPMODE, (Read_Reg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY | RF_OPMODE_LISTEN_ON);
			currentMode = RF_OPMODE_LISTEN_ON
			break;

		case RF69_MODE_SYNTH:
			Write_Reg(REG_OPMODE, (Read_Reg(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
			currentMode = RF_OPMODE_SYNTHESIZER;
			break;

		default:
			return;

	}

}



}
void transmit(uint8_t networkID, uint8_t nodeID, uint8_t buffer_length, const void* out_buffer, bool requestACK, book sendACK){
	//transmits message from FIFO

	//control byte
	if(requestACK == true){
		CTB = RFM69_CTL_REQACK;
	if(sendACK == true){
		CTB = RFM69_CTL_SENDACK;
	}

	setMode(RF69_MODE_STANDBY); //to prevent broadcasting before FIFO has been filled with our message

	//write to FIFO
	write_Reg(REG_FIFO,NULL); //writes the FIFO address with a bit that indicates that we want to write to the FIFO we are sending to 

	/*begining of message we will be sending */
	write_Reg(NULL, buffer_length);
	Write_Reg(NULL, networkID);
	write_Reg(NULL, nodeID);
	write_Reg(NULL, CTB);
	
	
	for(uint8_t i ==0; i < buffer_length; i++){   
		Write_Reg(Null, (uint8_t)buffer[i])      //one bit is sent for each cycle of the clock
	}
	/*end of message we will be sending */

	/*disable interupts*/
	ToggleInterrupts(0);

	/*initiate transmission*/
	setMode(RF69_MODE_TX);
	
	/*transmit for as long as we can, the limit is 1 second*/
	uint8_t txStartTime = millis();
	while(txStartTime - millis() < 1000){
		//wait
	}
	setMode(RF69_MODE_STANDBY);
	toggleInterrupts(1);

	/*stop transmission*/

		
	}


}
void ToggleInterrupts(uint8_t on_off){
	if(on_off == 1){
		//interrupts are turned on
		interrupts = 1;
	}
	if(on_off == 0){
		//interrupts are turned off
		interrupts = 0;
	}
	
	
}