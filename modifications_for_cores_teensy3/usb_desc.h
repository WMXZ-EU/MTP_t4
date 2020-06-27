
#elif defined(USB_MTPDISK_SERIAL)
  #define VENDOR_ID             0x16C0
  #define PRODUCT_ID		0x0476
  #define RAWHID_USAGE_PAGE	0xFFAB  // recommended: 0xFF00 to 0xFFFF
  #define RAWHID_USAGE		0x0200  // recommended: 0x0100 to 0xFFFF
  #define DEVICE_CLASS		0xEF
  #define DEVICE_SUBCLASS	0x02
  #define DEVICE_PROTOCOL	0x01

  #define MANUFACTURER_NAME     {'T','e','e','n','s','y','d','u','i','n','o'}
  #define MANUFACTURER_NAME_LEN 11
  #define PRODUCT_NAME          {'T','e','e','n','s','y',' ','M','T','P',' ','D','i','s','k','/','S','e','r','i','a','l'}
  #define PRODUCT_NAME_LEN      22
  #define EP0_SIZE              64

  #define NUM_ENDPOINTS         6
  #define NUM_USB_BUFFERS       20
  #define NUM_INTERFACE         3

  #define CDC_IAD_DESCRIPTOR    1
  #define CDC_STATUS_INTERFACE  0
  #define CDC_DATA_INTERFACE    1 // Serial
  #define CDC_ACM_ENDPOINT      1
  #define CDC_RX_ENDPOINT       2
  #define CDC_TX_ENDPOINT       3
  #define CDC_ACM_SIZE          16
  #define CDC_RX_SIZE           64
  #define CDC_TX_SIZE           64
  
  #define MTP_INTERFACE         2 // MTP Disk
  #define MTP_TX_ENDPOINT       4
  #define MTP_TX_SIZE           64
  #define MTP_RX_ENDPOINT       5
  #define MTP_RX_SIZE           64
  #define MTP_EVENT_ENDPOINT    6
  #define MTP_EVENT_SIZE        16
  #define MTP_EVENT_INTERVAL    10
  #define ENDPOINT1_CONFIG      ENDPOINT_TRANSMIT_ONLY
  #define ENDPOINT2_CONFIG      ENDPOINT_RECEIVE_ONLY
  #define ENDPOINT3_CONFIG      ENDPOINT_TRANSMIT_ONLY
  #define ENDPOINT4_CONFIG      ENDPOINT_TRANSMIT_ONLY
  #define ENDPOINT5_CONFIG      ENDPOINT_RECEIVE_ONLY
  #define ENDPOINT6_CONFIG      ENDPOINT_RECEIVE_ONLY
