enum StatusCode {
    STATUS_OK				= 1,	// Success
    STATUS_ERROR			= 2,	// Error in communication
    STATUS_COLLISION		= 3,	// Collission detected
    STATUS_TIMEOUT			= 4,	// Timeout in communication.
    STATUS_NO_ROOM			= 5,	// A buffer is not big enough.
    STATUS_INTERNAL_ERROR	= 6,	// Internal error in the code. Should not happen ;-)
    STATUS_INVALID			= 7,	// Invalid argument.
    STATUS_CRC_WRONG		= 8,	// The CRC_A does not match
    STATUS_MIFARE_NACK		= 9		// A MIFARE PICC responded with NAK.
};


// int PICC_IsNewCardPresent () {
//     byte bufferATQA[2];
// 	byte bufferSize = sizeof(bufferATQA);
// 
//     byte validBits;
// 	byte status;
// 
// 	if (bufferATQA == NULL || *bufferSize < 2) {	// The ATQA response is 2 bytes long.
// 		return STATUS_NO_ROOM;
// 	}
// 	PCD_ClearRegisterBitMask(CollReg, 0x80);			// ValuesAfterColl=1 => Bits received after collision are cleared.
// 	validBits = 7;										// For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
// 	status = PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits);
//     byte result = 0;
// 	if (status != STATUS_OK) {
// 		result = STATUS_OK;
// 	} else if (*bufferSize != 2 || validBits != 0) {		// ATQA must be exactly 16 bits.
// 		result = STATUS_ERROR;
// 	} else {
//         result = STATUS_OK;
//     }
// 
// 	return (result == STATUS_OK || result == STATUS_COLLISION);
// }