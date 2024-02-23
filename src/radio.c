
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mm_radio, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/irq.h>

#include "hal/nrf_radio.h"

enum {
	RECEIVER,
	TRANSMITTER,
};

static uint8_t packet[ 255 ]; // Packet to send

static const char *event_to_str(nrf_radio_event_t event) {
	switch (event) {
	case NRF_RADIO_EVENT_READY:
		return "READY";
	case NRF_RADIO_EVENT_ADDRESS:
		return "ADDRESS";
	case NRF_RADIO_EVENT_PAYLOAD:
		return "PAYLOAD";
	case NRF_RADIO_EVENT_END:
		return "END";
	case NRF_RADIO_EVENT_DISABLED:
		return "DISABLED";
	case NRF_RADIO_EVENT_DEVMATCH:
		return "DEVMATCH";
	case NRF_RADIO_EVENT_DEVMISS:
		return "DEVMISS";
	case NRF_RADIO_EVENT_RSSIEND:
		return "RSSIEND";
	case NRF_RADIO_EVENT_BCMATCH:
		return "BCMATCH";
	case NRF_RADIO_EVENT_CRCOK:
		return "CRCOK";
	case NRF_RADIO_EVENT_CRCERROR:
		return "CRCERROR";
	case NRF_RADIO_EVENT_FRAMESTART:
		return "FRAMESTART";
	case NRF_RADIO_EVENT_EDEND:
		return "EDEND";
	case NRF_RADIO_EVENT_CCAIDLE:
		return "CCIDLE";
	case NRF_RADIO_EVENT_CCABUSY:
		return "CCABUSY";
	case NRF_RADIO_EVENT_CCASTOPPED:
		return "CCASTOPPED";
	case NRF_RADIO_EVENT_RATEBOOST:
		return "RATEBOOST";
	case NRF_RADIO_EVENT_TXREADY:
		return "TXREADY";
	case NRF_RADIO_EVENT_RXREADY:
		return "RXREADY";
	case NRF_RADIO_EVENT_MHRMATCH:
		return "MHRMATCH";
	case NRF_RADIO_EVENT_SYNC:
		return "SYNC";
	case NRF_RADIO_EVENT_PHYEND:
		return "PHYEND";
	case NRF_RADIO_EVENT_CTEPRESENT:
		return "CTEPRESENT";
	default:
		return "???";
	}
}

static void dump_regs() {
	LOG_DBG("SHORTS:	%08x", NRF_RADIO->SHORTS);
	LOG_DBG("INTENSET:  %08x", NRF_RADIO->INTENSET);
	LOG_DBG("INTENCLR:  %08x", NRF_RADIO->INTENCLR);
	LOG_DBG("CRCSTATUS: %08x", NRF_RADIO->CRCSTATUS);
	LOG_DBG("RXMATCH:   %08x", NRF_RADIO->RXMATCH);
	LOG_DBG("RXCRC:     %08x", NRF_RADIO->RXCRC);
	LOG_DBG("DAI:       %08x", NRF_RADIO->DAI);
	LOG_DBG("PDUSTAT:   %08x", NRF_RADIO->PDUSTAT);
	LOG_DBG("CTESTATUS: %08x", NRF_RADIO->CTESTATUS);
	LOG_DBG("DFESTATUS: %08x", NRF_RADIO->DFESTATUS);
	LOG_DBG("PACKETPTR: %08x", NRF_RADIO->PACKETPTR);
	LOG_DBG("FREQUENCY: %08x", NRF_RADIO->FREQUENCY);
	LOG_DBG("TXPOWER:   %08x", NRF_RADIO->TXPOWER);
	LOG_DBG("MODE:      %08x", NRF_RADIO->MODE);
	LOG_DBG("PCNF0:     %08x", NRF_RADIO->PCNF0);
	LOG_DBG("PCNF1:     %08x", NRF_RADIO->PCNF1);
	LOG_DBG("BASE0:     %08x", NRF_RADIO->BASE0);
	LOG_DBG("PREFIX0:   %08x", NRF_RADIO->PREFIX0);
	LOG_DBG("PREFIX1:   %08x", NRF_RADIO->PREFIX1);
	LOG_DBG("TXADDRESS: %08x", NRF_RADIO->TXADDRESS);
	LOG_DBG("RXADDRESSES: %08x", NRF_RADIO->RXADDRESSES);
	LOG_DBG("CRCCNF:    %08x", NRF_RADIO->CRCCNF);
	LOG_DBG("CRCPOLY:   %08x", NRF_RADIO->CRCPOLY);
	LOG_DBG("CRCINIT:   %08x", NRF_RADIO->CRCINIT);
	LOG_DBG("TIFS:      %08x", NRF_RADIO->TIFS);
	LOG_DBG("RSSISAMPLE: %08x", NRF_RADIO->RSSISAMPLE);
	LOG_DBG("STATE:     %08x", NRF_RADIO->STATE);
	LOG_DBG("DATAWHITEIV: %08x", NRF_RADIO->DATAWHITEIV);
	LOG_DBG("BCC:       %08x", NRF_RADIO->BCC);
	LOG_DBG("DAB[%d]:    %08x", 0, NRF_RADIO->DAB[ 0 ]);
	LOG_DBG("DAB[%d]:    %08x", 1, NRF_RADIO->DAB[ 1 ]);
	LOG_DBG("DAB[%d]:    %08x", 2, NRF_RADIO->DAB[ 2 ]);
	LOG_DBG("DAB[%d]:    %08x", 3, NRF_RADIO->DAB[ 3 ]);
	LOG_DBG("DAB[%d]:    %08x", 4, NRF_RADIO->DAB[ 4 ]);
	LOG_DBG("DAB[%d]:    %08x", 5, NRF_RADIO->DAB[ 5 ]);
	LOG_DBG("DAB[%d]:    %08x", 6, NRF_RADIO->DAB[ 6 ]);
	LOG_DBG("DAB[%d]:    %08x", 7, NRF_RADIO->DAB[ 7 ]);
	LOG_DBG("DAP[%d]:    %08x", 0, NRF_RADIO->DAP[ 0 ]);
	LOG_DBG("DAP[%d]:    %08x", 1, NRF_RADIO->DAP[ 1 ]);
	LOG_DBG("DAP[%d]:    %08x", 2, NRF_RADIO->DAP[ 2 ]);
	LOG_DBG("DAP[%d]:    %08x", 3, NRF_RADIO->DAP[ 3 ]);
	LOG_DBG("DAP[%d]:    %08x", 4, NRF_RADIO->DAP[ 4 ]);
	LOG_DBG("DAP[%d]:    %08x", 5, NRF_RADIO->DAP[ 5 ]);
	LOG_DBG("DAP[%d]:    %08x", 6, NRF_RADIO->DAP[ 6 ]);
	LOG_DBG("DAP[%d]:    %08x", 7, NRF_RADIO->DAP[ 7 ]);
	LOG_DBG("DACNF:     %08x", NRF_RADIO->DACNF);
	LOG_DBG("MHRMATCHCONF: %08x", NRF_RADIO->MHRMATCHCONF);
	LOG_DBG("MHRMATCHMAS: %08x", NRF_RADIO->MHRMATCHMAS);
	LOG_DBG("MODECNF0:  %08x", NRF_RADIO->MODECNF0);
	LOG_DBG("SFD:     %08x", NRF_RADIO->SFD);
	LOG_DBG("EDCNT:   %08x", NRF_RADIO->EDCNT);
	LOG_DBG("EDSAMPLE: %08x", NRF_RADIO->EDSAMPLE);
	LOG_DBG("CCACTRL: %08x", NRF_RADIO->CCACTRL);
	LOG_DBG("DFEMODE: %08x", NRF_RADIO->DFEMODE);
	LOG_DBG("CTEINLINECONF: %08x", NRF_RADIO->CTEINLINECONF);
	LOG_DBG("DFECTRL1: %08x", NRF_RADIO->DFECTRL1);
	LOG_DBG("DFECTRL2: %08x", NRF_RADIO->DFECTRL2);
	LOG_DBG("SWITCHPATTERN: %08x", NRF_RADIO->SWITCHPATTERN);
	LOG_DBG("CLEARPATTERN: %08x", NRF_RADIO->CLEARPATTERN);
	LOG_DBG("PSEL.DFEGPIO[%d]: %08x", 0, NRF_RADIO->PSEL.DFEGPIO[ 0 ]);
	LOG_DBG("PSEL.DFEGPIO[%d]: %08x", 1, NRF_RADIO->PSEL.DFEGPIO[ 1 ]);
	LOG_DBG("PSEL.DFEGPIO[%d]: %08x", 2, NRF_RADIO->PSEL.DFEGPIO[ 2 ]);
	LOG_DBG("PSEL.DFEGPIO[%d]: %08x", 3, NRF_RADIO->PSEL.DFEGPIO[ 3 ]);
	LOG_DBG("PSEL.DFEGPIO[%d]: %08x", 4, NRF_RADIO->PSEL.DFEGPIO[ 4 ]);
	LOG_DBG("PSEL.DFEGPIO[%d]: %08x", 5, NRF_RADIO->PSEL.DFEGPIO[ 5 ]);
	LOG_DBG("PSEL.DFEGPIO[%d]: %08x", 6, NRF_RADIO->PSEL.DFEGPIO[ 6 ]);
	LOG_DBG("PSEL.DFEGPIO[%d]: %08x", 7, NRF_RADIO->PSEL.DFEGPIO[ 7 ]);
	LOG_DBG("DFEPACKET.PTR: %08x", NRF_RADIO->DFEPACKET.PTR);
	LOG_DBG("DFEPACKET.MAXCNT: %08x", NRF_RADIO->DFEPACKET.MAXCNT);
	LOG_DBG("DFEPACKET.AMOUNT: %08x", NRF_RADIO->DFEPACKET.AMOUNT);
	LOG_DBG("POWER: %08x", NRF_RADIO->POWER);
}

static void radio_config() {

	nrf_radio_power_set(NRF_RADIO, true);

	nrf_radio_mode_set(NRF_RADIO, RADIO_MODE_MODE_Ble_1Mbit);

	nrf_radio_packet_conf_t config = {
		.lflen		= 0,							// Length on air of LENGTH field in number of bits.
		.s0len		= 0,							// Length on air of S0 field in number of bytes.
		.s1len		= 0,							// Length on air of S1 field in number of bits.
		.s1incl		= RADIO_PCNF0_S1INCL_Automatic, // Include or exclude S1 field in RAM.
		.cilen		= 0,							// Length of code indicator - long range.
		.plen		= RADIO_PCNF0_PLEN_16bit,		// Length of preamble on air. Decision point: TASKS_START task.
		.crcinc		= false,						// Indicates if LENGTH field contains CRC or not.
		.termlen	= 0,							// Length of TERM field in Long Range operation.
		.maxlen		= 60,							// Maximum length of packet payload.
		.statlen	= 60,							// Static length in number of bytes.
		.balen		= 4,							// Base address length in number of bytes.
		.big_endian = false,						// On air endianness of packet.
		.whiteen	= true							// Enable or disable packet whitening.
	};
	nrf_radio_packet_configure(NRF_RADIO, &config);

	nrf_radio_datawhiteiv_set(NRF_RADIO, 0xAB);

	nrf_radio_base0_set(NRF_RADIO, 0xE7E7E7E7);
	nrf_radio_prefix0_set(NRF_RADIO, 0xE7);

	nrf_radio_txaddress_set(NRF_RADIO, 0);
	nrf_radio_rxaddresses_set(NRF_RADIO, 1);

	nrf_radio_crc_configure(NRF_RADIO, 2, RADIO_CRCCNF_SKIP_ADDR_Skip, 0x000AA0AA);
	nrf_radio_crcinit_set(NRF_RADIO, 0x00005050);

	nrf_radio_fast_ramp_up_enable_set(NRF_RADIO, true);

	nrf_radio_txpower_set(NRF_RADIO, RADIO_TXPOWER_TXPOWER_0dBm);
	nrf_radio_frequency_set(NRF_RADIO, 0);

	nrf_radio_packetptr_set(NRF_RADIO, packet);
}

/* static int radio_isr() { */
ISR_DIRECT_DECLARE(radio_isr) {

	__disable_irq();

	LOG_DBG("ISR");
	for (int i = 0x100; i <= 0x170; i += 4) {
		if (nrf_radio_event_check(NRF_RADIO, i)) {
			LOG_DBG("NRF_RADIO_EVENT: %12s (%03x)", event_to_str(i), i);
			nrf_radio_event_clear(NRF_RADIO, i);
		}
	}

	__enable_irq();
	return 0;
}

int radio_init() {

	for (int i = 0; i < 255; i++) {
		packet[ i ] = i;
	}

	radio_config();

	nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_END_MASK | NRF_RADIO_INT_READY_MASK | NRF_RADIO_INT_ADDRESS_MASK | NRF_RADIO_INT_PAYLOAD_MASK | NRF_RADIO_INT_DISABLED_MASK | NRF_RADIO_INT_TXREADY_MASK | NRF_RADIO_INT_RXREADY_MASK | NRF_RADIO_INT_CRCERROR_MASK | NRF_RADIO_INT_CRCOK_MASK | NRF_RADIO_INT_SYNC_MASK | NRF_RADIO_INT_PHYEND_MASK);

	IRQ_DIRECT_CONNECT(RADIO_IRQn, 0, radio_isr, 0);
	irq_enable(RADIO_IRQn);

	LOG_DBG("radio state: %d", nrf_radio_state_get(NRF_RADIO));

	uint8_t mode = TRANSMITTER;

	switch (mode) {
	case TRANSMITTER:
		nrf_radio_shorts_set(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK);
		NRF_RADIO->TASKS_TXEN = 1;
		for (;;) {
			k_msleep(1000);
			LOG_INF("radio state: %d", nrf_radio_state_get(NRF_RADIO));
			/* dump_regs(); */
			NRF_RADIO->TASKS_START = 1;
		}
		break;
	case RECEIVER:
		nrf_radio_dacnf_set(NRF_RADIO, 1, 1);
		nrf_radio_shorts_set(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK | NRF_RADIO_SHORT_END_START_MASK);
		NRF_RADIO->TASKS_RXEN  = 1;
		NRF_RADIO->TASKS_START = 1;
		k_msleep(100);
		for (;;) {
			k_msleep(1000);
			/* dump_regs(); */
			LOG_INF("radio state: %d", nrf_radio_state_get(NRF_RADIO));
		}
		break;
	}

	return 0;
}
