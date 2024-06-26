;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
;� Include File: PS2_5060.INC					      �
;�								      �
;� Purpose: Initialization code for the Personal Systems/2	      �
;�	     models 50 and 60.				 	      �
;�								      �
;� Returns: INIT_ERR						      �
;�	      Flag indicating if an error was detected. 	      �
;�	      DX = ptr to error message string if error.	      �
;�								      �
;�    This procedure is called to initialize the XMO and/or XMA/A     �
;�  card(s) on a PS/2 mod 50 or 60.  Any mix of multiple XMA and      �
;�  XMO cards are supported.  The XMA cards will be used in	      �
;�  'real' mode, meaning the virtual mode ports providing bank        �
;�  swapping are not used (since this only works on 1 XMA card).      �
;�  This procedure is not called if WSP's XMA/A device driver         �
;�  (INDXMAA.SYS) is installed.  In this case only the single XMA     �
;�  card is used for EMS, and it is spoken to in 'virtual' mode.      �
;�    The procedure searches each adapter slot for the presence of    �
;�  an XMA or a XMO card by checking the card ID.  It checks	      �
;�  the configuration registers on the cards to determine the amount  �
;�  of memory they contain.					      �
;�    The procedure will then calculate for the /E parameter.  This   �
;�  states how much of the extended memory the user wants for EMS.    �
;�  Extended memory will come from the top of the address range,      �
;�  and EMS will come off the bottom (i.e. starting at 1M+384K).      �
;�  Memory kept as extended has to be marked unusable in the Page     �
;�  Allocation List.  Translate table entries in extended memory      �
;�  are disabled for memory used for EMS.			      �
;�    Note that the /E parameter is only valid within this	      �
;�  procedure, i.e. for PS/2 mod 50 and 60's.  On family 1 machines,  �
;�  only the XMA 1 card is supported, and it doesn't come up as       �
;�  extended memory.  On mod 50 or 60 with the XMA/A driver, the      �
;�  driver takes all of the (uppermost) XMA card and resets CMOS.     �
;�  On the mod 80 with the XMA emulator, this XMA/extended split      �
;�  must be specified on the Emulator's parm line.                    �
;�								      �
;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸

include SYSVAR.INC			;system variables structure		;an007; dms;


			     ;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
			     ;�   XMA/A declares			       �
			     ;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
HI6BIT_MASK	EQU  00000011b		;Mask for 2 bit bank descriptor
LO2BIT_FLIP	EQU  00000011b		;Reverses bottom 2 bits in bank des
					; gives # of 1/2M in that bank
NUM_CONFR_BANKS EQU 3			;Number of memory banks described
					; by the XMA/A config register
					; Bank 4 is on the control reg.
X_CONF_REG_VAL	   DB	?		;temporary holder for XMAA's
					; config (memory size) register
X_CTRL_REG_VAL	   DB	?		;temporary holder for XMAA's
					; control (mem size bank 4) reg.
X_BLKS_PER_HALFM   DB  128		;4K blocks per half meg of memory
XMAA_NUM_BLOCKS DW	?		;temp for # of 4K blocks on xmaa   @RH2
			     ;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
			     ;�   Expanded Memory Option (XMO) declares	       �
			     ;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
NUM_CINFO_BANKS EQU 3			;# of complete memory banks (4-2)
					; described by the XMO card info reg
					; Bank 1 - hi 1/2 bit - info, lo CC/P
H_CARD_INFO_VAL    DB	?		;temporary holder for XMO card's
					; info (memory size) register
H_BLKS_PER_HALFM   DB	32		;16K XMO card blocks per 1/2M of mem
HLST_NUM_BLOCKS DW	?		;temp for # of 16K blocks on hlstr @RH3
			     ;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
			     ;�   /E option declares			       �
			     ;�    (used to set extended memory)	       �
			     ;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
ems_pgs_parm   dw    0			;temp value for /E parameter
e_parm_def     equ   0FFFFh		;default...take all for ems
MIN_EXTMEM_H   EQU   (1024+384)/16	;Translate table pointer for the lowest
MIN_EXTMEM_X   EQU   (1024+384)/4	; addr extended memory can start at on
BASE_MEM       EQU   1024		;Base planer memory			;an007; dms;
					; a PS/2 (16K XMO and 4K XMA)
PREV_EXT_PGS   DW    0			;Extended memory claimed by previous
					; drivers
NEEDED_EMS_PGS DW    ?			;Pages that will go for EMS use
CARDS_PGS      DW    ?			;Number of pages on card being checked
CARD_EXT_S16K  DW    ?			; and where its extended memory starts
					; expressed in 16K blocks

INIT_MOD_50_60	PROC

	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	SI
	PUSH	DI

	MOV	INIT_ERR,NO_ERROR	;Initialize error flag		   @RH4
	MOV	TOTAL_SYS_PAGES,0	;Init total number of pages in the @RH2
	MOV	NUM_MEM_CARDS,0 	; system & # of memory cards found @RH2
	XOR	DI,DI			;Clear offset into mem card table  @RH2

			;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
			;� Search for XMO cards			  	       �
			;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
	XOR	CX,CX			;Check all system slots starting   @RH2
					; at slot 0			    RH2
H_SLOT_SCAN:
	MOV	AL,CL			;Enable the specific slot by ORing @RH2
	OR	AL,SLOT_SETUP		; the slot (bits 0-2) with the	   @RH2
	OUT	96h,AL			; setup flag (bit 3).		   @RH2

	MOV	DX,CARD_ID_LO		;Read the signature ID of the card @RH2
	IN	AL,DX			;				   @RH2
	XCHG	AL,AH			;				   @RH2
	MOV	DX,CARD_ID_HI		;				   @RH2
	IN	AL,DX			;				   @RH2
HLST_CHECK:
	CMP	AX,HLST_CARD_ID 	;If it's a XMO card then 	   @RH3
	JNE	H_NEXT_SLOT		; calculate the amount of memory   @RH3
	CALL	HLST_MEM_ADD		; on the card			   @RH3
	MOV	WTT_CARD_SLOT,CL	;Set default slot # and card type  @RH5
	MOV	MEMCARD_MODE,HOLS_REAL	; for single card support	   @RH5

	MOV	BX,HLST_NUM_BLOCKS	;1 XMO card block = an EMS page

				       ;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
				       ;� Save info in the memory card table   �
				       ;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
	MOV	MEM_CARD_TABLE.CARD_ID[DI],AX	   ;Save the card ID and   @RH5
	MOV	MEM_CARD_TABLE.CARD_SLOT[DI],CL    ; slot # of this card   @RH5
	MOV	AX,TOTAL_SYS_PAGES		   ;Set # of the 1st EMS   @RH5
	MOV	MEM_CARD_TABLE.START_PG_NUM[DI],AX ; page this card maps   @RH5
	ADD	AX,BX				   ;Last page mapped =	   @RH5
	DEC	AX				   ; 1st pg + pages on	   @RH5
	MOV	MEM_CARD_TABLE.END_PG_NUM[DI],AX   ;  this card - 1.	   @RH5

	ADD	TOTAL_SYS_PAGES,BX		   ;Add card's pgs to tot. @RH5
	INC	NUM_MEM_CARDS			   ;Inc # of cards found   @RH5
	ADD	DI,TYPE MEM_CARD_STRUC		   ;Next entry in card	   @RH5
						   ; descriptor table	    RH5

H_NEXT_SLOT:
	INC	CL				 ;Check next adapter slot  @RH2
	CMP	CL,NUM_OF_SLOTS 		 ;Is it <= system slots?   @RH2
	JB	H_SLOT_SCAN			 ;Yes..check next slot		;ac000; dms;
						 ;No fall through loop	    RH2

			;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
			;� Search for XMA/A cards			       �
			;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
	XOR	CX,CX			;Check all slots starting at 0	   @RH2
X_SLOT_SCAN:
	MOV	AL,CL			;Enable the specific slot by ORing @RH2
	OR	AL,SLOT_SETUP		; the slot (bits 0-2) with the	   @RH2
	OUT	96h,AL			; setup flag (bit 3).		   @RH2

	MOV	DX,CARD_ID_LO		;Read the signature ID of the card @RH2
	IN	AL,DX			;				   @RH2
	XCHG	AL,AH			;				   @RH2
	MOV	DX,CARD_ID_HI		;				   @RH2
	IN	AL,DX			;				   @RH2
XMAA_CHECK:
	CMP	AX,XMAA_CARD_ID 	;If it's an XMA/A card then        @RH2
	JNE	X_NEXT_SLOT		; calculate the amount of memory   @RH2
	CALL	XMAA_MEM_ADD		; on the card			   @RH2
	MOV	WTT_CARD_SLOT,CL	;Set default slot # and card type  @RH5
	MOV	MEMCARD_MODE,XMAA_REAL	; for single card support	   @RH5

	MOV	BX,XMAA_NUM_BLOCKS	;Divide the # of 4K XMA/A blocks   @RH2
	SHR	BX,1			; by 4 to get number or 16K EMS    @RH2
	SHR	BX,1			; pages on this card		   @RH2

				       ;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
				       ;� Save info in the memory card table   �
				       ;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
	MOV	MEM_CARD_TABLE.CARD_ID[DI],AX	   ;Save the card ID and   @RH5
	MOV	MEM_CARD_TABLE.CARD_SLOT[DI],CL    ; slot # of this card   @RH5
	MOV	AX,TOTAL_SYS_PAGES		   ;Set # of the 1st EMS   @RH5
	MOV	MEM_CARD_TABLE.START_PG_NUM[DI],AX ; page this card maps   @RH5
	ADD	AX,BX				   ;Last page mapped =	   @RH5
	DEC	AX				   ; 1st pg + pages on	   @RH5
	MOV	MEM_CARD_TABLE.END_PG_NUM[DI],AX   ;  this card - 1.	   @RH5

	ADD	TOTAL_SYS_PAGES,BX		   ;Add card's pgs to tot. @RH5
	INC	NUM_MEM_CARDS			   ;Inc # of cards found   @RH5
	ADD	DI,TYPE MEM_CARD_STRUC		   ;Next entry in card	   @RH5
						   ; descriptor table	    RH5

X_NEXT_SLOT:
	INC	CL				 ;Check next adapter slot  @RH2
	CMP	CL,NUM_OF_SLOTS 		 ;Is it <= system slots?   @RH2
	JB	X_SLOT_SCAN			 ;Yes..check next slot		;ac000; dms;
						 ;No fall through loop	    RH2

	CMP	TOTAL_SYS_PAGES,0	;If one or more cards are found    @RH4
	JA	CALC_EXTENDED		; then everythang's cool so far    @RH4
	    MOV     INIT_ERR,ERROR		 ;Else no card...set	   @RH4
	    MOV     DX,OFFSET NOT_FOUND_MSG	 ; 1st part of error msg   @RH4
	    JMP     INIT_50_60_RET		 ; for no card found	   @RH4

			;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
			;� Calculate /E parameter - amount of memory for EMS   �
			;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
					;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
					;� Find ext mem addr of bottom card   �
					;� in case card memory ever starts at �
					;� something other than 1M + 384K     �
					;� (i.e. if more planar memory is     �
					;�  added or a new unsupported card   �
					;�  comes in below XMO card)	      �
					;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
CALC_EXTENDED:
	push	es			;this call kills these regs		;an007; dms;
	push	bx			;					;an007; dms;

	mov	ah,52h			;get the sysvars ptr			;an007; dms;
	int	21h			; to get total ext. memory at boot	;an007; dms;
	mov	ax,word ptr es:[bx].SYSI_Ext_Mem	;			;an007; dms;

	pop	bx			;restore regs				;an007; dms;
	pop	es			;					;an007; dms;

	add	ax,Base_Mem		; + base memory 			;an007; dms;
	MOV	CL,4			; convert to 16Kb pages 		;an007; dms;
	SHR	AX,CL			;					;an007; dms;
	sub	ax,cs:Total_Sys_Pages	;get page where card begins		;an007; dms;

	xor	di,di			;init. index value			;an008; dms
	CMP	MEM_CARD_TABLE.CARD_ID[DI],HLST_CARD_ID ;If 1st card holst @RH4
	JE	FIND_H_1ST_TT				; then get hlst TT @RH4
FIND_X_1ST_TT:				;XMAA is 1st card (lowest ext mem) @RH4
;;;;;	MOV	AX,MIN_EXTMEM_X 	;Set XMAA TT ptr at 1.384M and	   @RH4
	shl	ax,1			;					;an000; dms;
	shl	ax,1			;					;an000; dms;
FIND_X_1ST_LOOP:			; search for start of ext mem	   @RH4
	CALL	X_READ_TT		;Read trans tbl data at this addr  @RH4
	CMP	BX,XMA_TT_INHIBIT	;If not inhibitted then mem here.  @RH4
	JE	FIND_X_1ST_NEXT 	;Divide the 4K translate table ptr @RH4
	MOV	CL,2			; by 4 to convert it to 16K format @RH4
	SHR	AX,CL			; AX = start of ext mem (in 16K)   @RH4
	JMP	FIRST_EXT_FOUND 	;				   @RH4
FIND_X_1ST_NEXT:			;				   @RH4
	INC	AX			;Else no mem...inc TT ptr and see  @RH4
	JMP	FIND_X_1ST_LOOP 	; if ext mem starts at next 4K	   @RH4

FIND_H_1ST_TT:				;XMO is 1st card (lowest ext mem)  @RH4
;;;;;	MOV	AX,MIN_EXTMEM_H 	;Set hlst TT ptr at 1.384M and	   @RH4
FIND_H_1ST_LOOP:			; search for start of ext mem	   @RH4
	CALL	H_READ_TT		;Read trans tbl data at this addr  @RH4
	CMP	BL,H_TT_INHIBIT 	;If not inhibitted then mem here.  @RH4
	JNE	FIRST_EXT_FOUND 	; found start of card ext mem	   @RH4
	INC	AX			;Else no mem...inc TT ptr and see  @RH4
	JMP	FIND_H_1ST_LOOP 	; if ext mem starts at next 16K    @RH4
FIRST_EXT_FOUND:
	MOV	CARD_EXT_S16K,AX	;Save the start of ext mem ptr	   @RH4
					;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
					;� Calc pages for extended memory     �
					;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
	MOV	AH,88H			;Go to BIOS and find amount of	   @RH4
	INT	15H			; extended memory (assume previous @RH4
	MOV	CL,4			; drivers have hooked INT 15h)	   @RH4
	SHR	AX,CL			;Get # of 16k pages		   @RH4

	MOV	BX,CARD_EXT_S16K	;Pages of extended memory used	   @RH4
	ADD	BX,TOTAL_SYS_PAGES	; by previous drivers = 	   @RH4
	SUB	BX,1024/16		; what we know is the # of ext	   @RH4
	SUB	BX,AX			; pages minus what BIOS tells us   @RH4
	MOV	PREV_EXT_PGS,BX 	;				   @RH4

	CMP	EMS_PGS_PARM,E_PARM_DEF ;If no /E parm specified then	   @RH4
	JNE	CHECK_E_PARM		; use the remaining pages for EMS  @RH4
	MOV	AX,TOTAL_SYS_PAGES	; (total on cards minus previously @RH4
	cmp	Prev_Ext_Pgs,ax 	;Previous ext pages >= avail on cards?	;an000; dms;
	jae	Default_Mem_Err_Exit	;yes - we have used the whole card	;an000; dms;
	SUB	AX,PREV_EXT_PGS 	; used for extended memory)	   @RH4
	MOV	NEEDED_EMS_PGS,AX	;Set counter for marking PAL and TT@RH4
	JMP	SHORT MARK_EXT_IN_PAL

Default_Mem_Err_Exit:

	mov	Init_Err,Error		;flag an error occurred 		;an000; dms;
	lea	dx,No_EMS_Memory	;no memory on cards left		;an000; dms;
	jmp	Init_50_60_Ret		;exit routine				;an000; dms;

CHECK_E_PARM:				;Else test user specified # EMS pgs@RH4
	MOV	BX,EMS_PGS_PARM 	;Set counter for marking Page	   @RH4
	MOV	NEEDED_EMS_PGS,BX	; Allocation List and TT entries   @RH4
	MOV	AX,TOTAL_SYS_PAGES	;If the requested EMS pages are    @RH4
	SUB	AX,PREV_EXT_PGS 	; more than what's left            @RH4
	CMP	AX,EMS_PGS_PARM 	; (total pages minus pages	   @RH4
	JGE	MARK_EXT_IN_PAL 	; used by other drivers) then set  @RH4
	    MOV     INIT_ERR,ERROR	; an error condition flag	   @RH4
	    LEA     DX,REQ_EMS_ERR_MSG	; Set first part of error message  @RH4
	    JMP     INIT_50_60_RET	;				   @RH4

					;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
					;� Mark PAL for extended memory pages �
					;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
MARK_EXT_IN_PAL:
	xor	di,di			;start at bottom of PAL to init EXT	;an002; dms;
MARK_NEW_IN_PAL:
	MOV	CX,TOTAL_SYS_PAGES	;Loop for the 'new' extended pages @RH4
	SUB	CX,NEEDED_EMS_PGS	; (left over from /E parm and not  @RH4
	SUB	CX,PREV_EXT_PGS 	; reserved by a previous driver)   @RH4
	MOV	RESR_EXT_PGS,CX 	;				   @RH4
	CMP	CX,0			; This assumes that others anyone  @RH4
	JE	MARK_PREV_IN_PAL	; using extended memory after us   @RH4
MARK_NEW_LP:				; will take it from the top	   @RH4
	MOV	PAGE_ALLOC_LIST[DI],RESR_EXT	 ;Place a 'RE' in the PAL  @RH4
	ADD	DI,TYPE PAGE_ALLOC_LIST 	 ; (Reserved Extended) for @RH4
	LOOP	MARK_NEW_LP			 ; these entries	   @RH4

MARK_PREV_IN_PAL:
	MOV	AX,NEEDED_EMS_PGS	;Set offset into Page Alloc List   @RH4
	add	ax,Resr_Ext_Pgs 	;get end of area for EMS pages		;an002; dms;
	MOV	DX,TYPE PAGE_ALLOC_LIST ; for the page where extended mem  @RH4
	MUL	DX			; starts.  This is done by skipping@RH4
	MOV	DI,AX			; over the EMS pages on bottom	   @RH4
	MOV	CX,PREV_EXT_PGS 	;Loop for the previous extended    @RH4
	CMP	CX,0			; memory pages (if any).  This is  @RH4
	JE	DIS_EMS_TT		; ext mem claimed before we load   @RH4
MARK_PREV_LP:				;				   @RH4
	MOV	PAGE_ALLOC_LIST[DI],PREV_EXT	 ;Place a 'PE' in the PAL  @RH4
	ADD	DI,TYPE PAGE_ALLOC_LIST 	 ; (Previous Extended) for @RH4
	LOOP	MARK_PREV_LP			 ; these entries	   @RH4
					;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
					;� Disable translate table entries    �
					;� in extended memory for memory      �
					;� used as EMS			      �
					;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸

DIS_EMS_TT:

	mov	cx,Resr_Ext_Pgs 		;get extended page count	;ac008; dms;
						;1 based page count

	xor	di,di				;set mem card table ptr 	;an002; dms;

Card_Find_Chk:

	cmp	cx,Mem_Card_Table.End_Pg_Num[di];page > ending page on card?	;ac008; dms;
	ja	Card_Find_Loop			;yes - next card please 	;an002; dms;
	add	Card_Ext_S16K,cx		;get 1st. avail page		;an008; dms;
	mov	cx,Mem_Card_Table.End_Pg_Num[di];Calc # pages remaining on card ;an002; dms;
	sub	cx,Resr_Ext_Pgs 		;				;ac008; dms;
	inc	cx				;				;an002; dms;
	jmp	Calc_Cards_EMS			;				;an002; dms;

Card_Find_Loop:

	add	di,Type Mem_Card_Struc		;next pointer			;an002; dms;
	jmp	Card_Find_Chk			;continue loop			;an002; dms;


CALC_CARDS_EMS:

	MOV	CARDS_PGS,CX			      ; 		   @RH4

					;Calc # of pages to inhibit for.   @RH4
	CMP	CX,NEEDED_EMS_PGS	;If the card has less pgs than	   @RH4
	JBE	ADJUST_NEEDED_EMS	; # needed for EMS then just loop  @RH4
	MOV	CX,NEEDED_EMS_PGS	; for card.  Else loop for needed. @RH4

ADJUST_NEEDED_EMS:
	SUB	NEEDED_EMS_PGS,CX	;Remaining EMS pages to inhibit    @RH4
					; after this card is taken care of @RH4

	MOV	AL,MEM_CARD_TABLE.CARD_SLOT[DI]  ;Activate the slot of	   @RH4
	OR	AL,SLOT_SETUP			 ; the card and set ptr    @RH4
	OUT	96h,AL				 ; to where it's ext mem   @RH4
	MOV	AX,CARD_EXT_S16K		 ; starts		   @RH4

	CMP	MEM_CARD_TABLE.CARD_ID[DI],HLST_CARD_ID    ;Test card type @RH4
	JE	H_INHIBIT_EMS				   ;		   @RH4
X_INHIBIT_EMS:				;Inhibit TT on XMA/A for EMS	   @RH4
	PUSH	CX			;Save ctr for # EMS on this card   @RH4
	MOV	CL,2			;Convert 16K start of EMS ptr to   @RH4
	SHL	AX,CL			; 4K XMA/A translate table format  @RH4
	POP	CX			;Restore # EMS pages counter	   @RH4
X_INH_EMS_PGS:				;----Loop for all XMA EMS pages----@RH4
	PUSH	CX			;Save # EMS pages ctr		   @RH4
	MOV	CX,BLOCKS_PER_PAGE	;Loop for all XMA blocks in a pg   @RH4
X_INH_ONE_PAGE: 			;----Loop for one XMA EMS page-----@RH4
	CALL	X_INH_FOR_EMS		;Inhibit TT entry (AX = 4K ptr)    @RH4
	INC	AX			;Next XMA block 		   @RH4
	LOOP	X_INH_ONE_PAGE		;				   @RH4
	POP	CX			;Restore ctr for # EMS pgs on card @RH4
	LOOP	X_INH_EMS_PGS		;Loop for # EMS pgs on card	   @RH4
	JMP	short next_card_ems	;Now go fix that page alloc table  @RH4

H_INHIBIT_EMS:				;Inhibit TT on XMO for EMS	   @RH4
	CALL	H_INH_FOR_EMS		;Inhibit one TT entry per EMS page @RH4
	INC	AX			;Next 16K pointer		   @RH4
	LOOP	H_INHIBIT_EMS		; and do it again yahoooooo	   @RH4

NEXT_CARD_EMS:
	CMP	NEEDED_EMS_PGS,0	;If this card had the rest of the  @RH4
	JE	INIT_50_60_RET		; EMS pages then done disabling TT @RH4
					;Else EMS on next card.  Set ptr   @RH4
	MOV	AX,CARDS_PGS		; to where next card's ext mem     @RH4
	ADD	CARD_EXT_S16K,AX	; starts (in units of 16K) and save@RH4

	ADD	DI,TYPE MEM_CARD_STRUC	;Next entry in the memory card	   @RH4

	mov	cx,Mem_Card_Table.End_Pg_Num[di]	;calc pages on		;an002; dms;
	sub	cx,Mem_Card_Table.Start_Pg_Num[di]	;card			;an002; dms;
	inc	cx					;			;an002; dms;

	JMP	CALC_CARDS_EMS		; table.			   @RH4

INIT_50_60_RET:

	MOV	AX,TOTAL_SYS_PAGES	;Total EMS pages in the system =   @RH4
	SUB	AX,PREV_EXT_PGS 	; pages on cards - amount used	   @RH4
	SUB	AX,RESR_EXT_PGS 	; as extended memory		   @RH4
	MOV	TOTAL_SYS_PAGES,AX	;Initialize values for: 	   @RH4
	MOV	TOTAL_EMS_PAGES,AX	; EMS pages - pages to back planar @RH4
	MOV	FREE_PAGES,AX		; EMS pages free for applications  @RH4

	MOV	AL,0			;Reset the slot ID		   @RH5
	OUT	96h,AL			;				   @RH5

	POP	DI
	POP	SI
	POP	CX
	POP	BX
	POP	AX
	RET
INIT_MOD_50_60	ENDP

;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
;� HLST_MEM_ADD subprocedure					      �
;�   Calculates the amount of memory on the XMO card		      �
;�     on entry: (CL) = card slot #				      �
;�		 card is in setup mode				      �
;�								      �
;�  The following describes how to read memory size, where the 2 bit  �
;�   pattern indicates SIP size.  There are 2 SIPs per bank.	      �
;�								      �
;�   Reg:   Card Info			      Channel Check, Presence �
;�   Port:   102h			       105h		      �
;�   SIPS:   bank 4  bank 3  bank 2  bank 1    bank 1		      �
;�   Bit:     7  6    5  4    3  2    1 	   0		      �
;�   -----------------------------------------------------------------�
;�	      1  1    1  1    1  1    1 	   1  No memory,error �
;�	      1  0    1  0    1  0    1 	   0  256K	      �
;�	      0  1    0  1    0  1    0 	   1  512K	      �
;�	      0  0    0  0    0  0    0 	   0  1M	      �
;�								      �
;� Note that for bank 0, 102's bit 1 forms the upper bit and          �
;�  105's bit 0 forms the lower of the 2 bit presence pattern.        �
;�  Therefore, if the pattern is '10'B, then the bank has 256K.       �
;�								      �
;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
HLST_MEM_ADD  PROC
	PUSH	DX			;				   @RH3
	PUSH	CX			;				   @RH3
	PUSH	AX			;				   @RH3

	MOV	HLST_NUM_BLOCKS,0	;Init # of 16K blocks on card	   @RH3

	MOV	DX,H_CARD_INFO		;Read & store card info reg (102h) @RH3
	IN	AL,DX			; bits 7-2 describe banks 4-2	   @RH3
	AND	AL,11111110B		; bit 1 upper half of bank 1 descr @RH3
	MOV	H_CARD_INFO_VAL,AL	; (don't care about sleep bit 0)   @RH3
	MOV	DX,H_CC_PRES		;Read chan. check & presence (105) @RH3
	IN	AL,DX			; for bit 0 - lower half of bank 1 @RH3
	AND	AL,00000001B		; Turn off all other bits	   @RH3
	OR	AL,H_CARD_INFO_VAL	;Join 2 bits - bank 1 descriptor   @RH3

	XOR	AL,LO2BIT_FLIP		;Flip these 2 to get # of 1/2 Megs @RH3
					; in bank 1 of XMO card		   @RH3
	AND	AL,HI6BIT_MASK		;Clear other bits		   @RH3
	CMP	AL,LO2BIT_FLIP		;Are both bits on (i.e. 1M sips)?  @RH3
	JNE	H_B1_HMEG_OK		;No  # of 1/2 M for bank OK (0,1,2)@RH3
	INC	AL			;Yes # of 1/2 M should be 4 not 3  @RH3
H_B1_HMEG_OK:
	MUL	H_BLKS_PER_HALFM	;Multiply by # of 16k Blocks per   @RH3
					; half meg to get bank's blocks    @RH3
	ADD	HLST_NUM_BLOCKS,AX	;Add bank 1 to the total	   @RH3

	SHR	H_CARD_INFO_VAL,1	;Shift bank 4-2 descriptors to	   @RH3
	SHR	H_CARD_INFO_VAL,1	; bits 0-5			   @RH3

	MOV	CX,NUM_CINFO_BANKS	; Loop for banks accounted for by  @RH3
					; the XMO card info register	   @RH3
HLST_MEM_LOOP:
	MOV	AL,H_CARD_INFO_VAL	;Get 2 bit bank descriptor & flip  @RH3
	XOR	AL,LO2BIT_FLIP		; the 2 bits.  Bits now indicate   @RH3
					; the # of 1/2 meg in the bank	   @RH3
	AND	AL,HI6BIT_MASK		;Ignore other banks		   @RH3
	CMP	AL,LO2BIT_FLIP		;Are both bits on (i.e. 1M sips)?  @RH3
	JNE	H_B42_HMEG_OK		;No  # of 1/2 M for bank OK (0,1,2)@RH3
	INC	AL			;Yes # of 1/2 M should be 4 not 3  @RH3
H_B42_HMEG_OK:
	MUL	H_BLKS_PER_HALFM	;Multiply by # of 4k blocks per    @RH3
					; half meg to get bank's blocks    @RH3
	ADD	HLST_NUM_BLOCKS,AX	;Add XMO banks 4-2 to total	   @RH3
	SHR	H_CARD_INFO_VAL,1	;Get next bank			   @RH3
	SHR	H_CARD_INFO_VAL,1	;				   @RH3
	LOOP	HLST_MEM_LOOP		;				   @RH3

	POP	AX			;				   @RH3
	POP	CX			;				   @RH3
	POP	DX			;				   @RH3

	RET
HLST_MEM_ADD  ENDP

;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
;� XMAA_MEM_ADD subprocedure					      �
;�   Calculates the amount of memory on the XMAA card		      �
;�     on entry: (CL) = card slot #				      �
;�		 card is in setup mode				      �
;�								      �
;�  The following describes how to read memory size, where the 2 bit  �
;�   pattern indicates SIP size.  There are 2 SIPs per bank.	      �
;�								      �
;�   Reg:   Control Reg    Config, Channel Check reg		      �
;�   Port:   102h	    105h				      �
;�   SIPS:   bank 4	    bank 3  bank 2  bank 1		      �
;�   Bit:     7  6	     5	4    3	2    1	0		      �
;�   -------------------------------------------------		      �
;�	      1  1	     1	1    1	1    1	1  No memory or error �
;�	      1  0	     1	0    1	0    1	0  256K 	      �
;�	      0  1	     0	1    0	1    0	1  512K 	      �
;�	      0  0	     0	0    0	0    0	0  1M		      �
;�								      �
;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
XMAA_MEM_ADD  PROC
	PUSH	DX			;				   @RH2
	PUSH	CX			;				   @RH2
	PUSH	AX			;				   @RH2

					;Insure XMA/A init ROM is disabled @RH2
					;When the high byte of the TT data @RH2
	MOV	DX,X_CTRL_REG
	IN	AL,DX			; is read or written using port    @RH2
	MOV	X_CTRL_REG_VAL,AL	  ;  SAVE FOR MEM COUNT
	AND	AL,CR_ROMSLEEP_DIS	; 104h (real mode), the upper	   @RH2
	OUT	DX,AL			; nibble has info for setting the  @RH2
					; initialization ROM addresses. To @RH2
					; insure this causes no problems,  @RH2
					; disable XMA/A ROM by clearing    @RH2
					; ROM sleep bit in the control reg @RH2

	MOV	XMAA_NUM_BLOCKS,0	;				   @RH2

	MOV	AL,X_CTRL_REG_VAL	;Get bits 6&7 of control register  @RH2
	MOV	CL,6			; to get memory configuration of   @RH2
	SHR	AL,CL			; bank 4			   @RH2
	XOR	AL,LO2BIT_FLIP		;Flip these 2 to get # of 1/2M	   @RH2
					; in bank 4 of XMA/A card	   @RH2
	AND	AL,HI6BIT_MASK		;Clear other bits		   @RH2
	CMP	AL,LO2BIT_FLIP		;Are both bits on (i.e. 1M sips)?  @RH2
	JNE	CTRL_HMEG_OK		;No  # of 1/2 M for bank OK (0,1,2)@RH2
	INC	AL			;Yes # of 1/2 M should be 4 not 3  @RH2
CTRL_HMEG_OK:
	MUL	X_BLKS_PER_HALFM	;Multiply by # of 4k blocks per    @RH2
					; half meg to get bank's blocks    @RH2
	ADD	XMAA_NUM_BLOCKS,AX


	MOV	DX,X_CONF_REG		;Read the config, channel check    @RH2
	IN	AL,DX			; register to get mem size of	   @RH2
	MOV	X_CONF_REG_VAL,AL	; banks 1-3			   @RH2

	MOV	CX,NUM_CONFR_BANKS	; Loop for banks accounted for by  @RH2
					; the config register		   @RH2
XMAA_MEM_LOOP:
	MOV	AL,X_CONF_REG_VAL	;Get 2 bit bank descriptor & flip  @RH2
	XOR	AL,LO2BIT_FLIP		; the 2 bits.  Bits now indicate   @RH2
					; the # of 1/2 meg in the bank	   @RH2
	AND	AL,HI6BIT_MASK		;Ignore other banks		   @RH2
	CMP	AL,LO2BIT_FLIP		;Are both bits on (i.e. 1M sips)?  @RH2
	JNE	CONF_HMEG_OK		;No  # of 1/2 M for bank OK (0,1,2)@RH2
	INC	AL			;Yes # of 1/2 M should be 4 not 3  @RH2
CONF_HMEG_OK:
	MUL	X_BLKS_PER_HALFM	;Multiply by # of 4k blocks per    @RH2
					; half meg to get bank's blocks    @RH2
	ADD	XMAA_NUM_BLOCKS,AX	;				   @RH2
	SHR	X_CONF_REG_VAL,1	;Get next bank			   @RH2
	SHR	X_CONF_REG_VAL,1	;				   @RH2
	LOOP	XMAA_MEM_LOOP		;				   @RH2

	POP	AX			;				   @RH2
	POP	CX			;				   @RH2
	POP	DX			;				   @RH2

	RET
XMAA_MEM_ADD  ENDP

;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
;� H_READ_TT subprocedure					      �
;�   Reads the contents of a translate table entry on a XMO card      �
;�     on entry: (AX) = Translate table pointer 		      �
;�     on exit:  (BL) = Data (byte) at that entry		      �
;�								      �
;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
H_READ_TT	PROC
	PUSH	AX			;Save the TT pointer		   @RH4
	MOV	DX,H_TTPTR_LO		;Set the low and high bytes of	   @RH4
	OUT	DX,AL			; the XMO translate table	   @RH4
	XCHG	AL,AH			; pointer, then read the value	   @RH4
	MOV	DX,H_TTPTR_HI		; of the data for that entry	   @RH4
	OUT	DX,AL			;				   @RH4
	MOV	DX,H_TTDATA		;				   @RH4
	IN	AL,DX			;Read the data into AL		   @RH4
	MOV	BL,AL			; and store it in BL		   @RH4
	POP	AX			;Restore TT pointer		   @RH4
	RET				;				   @RH4
H_READ_TT	ENDP

;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
;� X_READ_TT subprocedure					      �
;�   Reads the contents of a translate table entry on an XMA/A card   �
;�     on entry: (AX) = Translate table pointer 		      �
;�     on exit:  (BX) = Data (12 bits) at that entry		      �
;�								      �
;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
X_READ_TT	PROC
	PUSH	AX			;Save the TT pointer		   @RH4
	MOV	DX,RM_TTPTR_LO		;Set the low and high bytes of	   @RH4
	OUT	DX,AL			; the XMA/A translate table	   @RH4
	XCHG	AL,AH			; pointer, then read the value	   @RH4
	MOV	DX,RM_TTPTR_HI		; of the data for that entry	   @RH4
	OUT	DX,AL			;				   @RH4

	MOV	DX,RM_TTDATA_HI 	;Read 12 bit TT data high byte	   @RH4
	IN	AL,DX			; first, then read low byte.	   @RH4
	XCHG	AL,AH			;				   @RH4
	MOV	DX,RM_TTDATA_LO 	;				   @RH4
	IN	AL,DX			;				   @RH4
	MOV	BX,AX			;Store result in BX		   @RH4
	AND	BX,XMA_TT_MASK		;Turn off useless upper 4 bits	   @RH4
	POP	AX			;Restore TT pointer		   @RH4
	RET				;				   @RH4
X_READ_TT	ENDP

;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
;� H_INH_FOR_EMS subprocedure					      �
;�   Inhibits a single translate table entry of extended memory       �
;�   on a XMO card.  This entry (16K) is for use by EMS.	      �
;�     on entry: (AX) = XMO Translate table pointer (# of K / 16)     �
;�								      �
;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
H_INH_FOR_EMS	PROC
	PUSH	AX			;Save the TT pointer		   @RH4
	MOV	DX,H_TTPTR_LO		;Set the low and high bytes of	   @RH4
	OUT	DX,AL			; the XMO translate table	   @RH4
	XCHG	AL,AH			; pointer,			   @RH4
	MOV	DX,H_TTPTR_HI		;				   @RH4
	OUT	DX,AL			;				   @RH4

	MOV	AL,H_TT_INHIBIT 	;AL = XMO TT inhibit data	   @RH4
	MOV	DX,H_TTDATA		;Inhibit this TT entry so that	   @RH4
	OUT	DX,AL			; it is no longer extended memory  @RH4
	POP	AX			;Restore TT pointer		   @RH4
	RET				;				   @RH4
H_INH_FOR_EMS	ENDP

;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
;� X_INH_FOR_EMS subprocedure					      �
;�   Inhibits a single translate table entry of extended memory       �
;�   on a XMA/A card.  This entry (4K) is for use by EMS.	      �
;�     on entry: (AX) = XMA/A Translate table pointer (# of K / 4)    �
;�								      �
;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
X_INH_FOR_EMS	PROC
	PUSH	AX			;Save the TT pointer		   @RH4
	MOV	DX,RM_TTPTR_LO		;Set the low and high bytes of	   @RH4
	OUT	DX,AL			; the XMA/A translate table	   @RH4
	XCHG	AL,AH			; pointer, then read the value	   @RH4
	MOV	DX,RM_TTPTR_HI		; of the data for that entry	   @RH4
	OUT	DX,AL			;				   @RH4

	MOV	AX,XMA_TT_INHIBIT	;AX = XMA 12 bit TT inhibit data   @RH4
	MOV	DX,RM_TTDATA_LO 	;Write 12 bit TT data low byte	   @RH4
	OUT	DX,AL			; first, then write high byte.	   @RH4
	XCHG	AL,AH			;				   @RH4
	MOV	DX,RM_TTDATA_HI 	;				   @RH4
	OUT	DX,AL			;				   @RH4
	POP	AX			;Restore TT pointer		   @RH4
	RET				;				   @RH4
X_INH_FOR_EMS	ENDP

;旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
;� ADJUST_CMOS subprocedure					      �
;�   Reset the CMOS value for amount of extended memory.  The	      �
;�   memory off the 'top' (upper addresses) is used by EMS.           �
;�     on entry:						      �
;�	CARD_EXT_S16K = First 16K translate table pointer past the    �
;�			top of the last card.  Example - One 2M card  �
;�			that started at 1M+384K, AX = 58h + 80H = D8h �
;�								      �
;�								      �
;읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
ADJUST_CMOS	PROC
CMOS_LO EQU  70h			;Port addrs of CMOS controller
CMOS_HI EQU  71h

	MOV	BX,CARD_EXT_S16K	;Convert to 1st 16K TT ptr past    @RH4
	SUB	BX,ems_pgs_parm 	; entries reserved for ext mem	   @RH4
	SUB	BX,1024/16		;Convert to # of 16K above 1M
	MOV	CL,4			;Multiply by 16 to get # of K
	SHL	BX,CL			; above 1 Megabyte

	MOV	AX,8800h		;Get BIOS' current value for #
	INT	15h			; of K extended (above 1M)

	CMP	AX,BX			;If already set below what we think
	JBE	CMOS_RET		; think then don't adjust it

	PUSHF				;Else adjust CMOS...save state of
	CLI				; interrupts and disable
	MOV	AL,0B5h 		;Select NMI off byte 35
	OUT	CMOS_LO,AL		;Write to CMOS controller
	JMP	$+2			;		  delay
	MOV	AL,BL			;Write low data byte to CMOS
	OUT	CMOS_HI,AL		;
	JMP	$+2			;

	MOV	AL,0B6h 		;Select NMI off byte 36
	OUT	CMOS_LO,AL		;Write to CMOS controller
	JMP	$+2			;		  delay
	MOV	AL,BH			;Write high data byte to CMOS
	OUT	CMOS_HI,AL		;
	JMP	$+2			;

	MOV	AL,0Fh			;Select NMI on byte 0f
	OUT	CMOS_LO,AL		;Write to CMOS controller
	JMP	$+2			;		   delay
	IN	AL,CMOS_HI		;Reset CMOS like BIOS does
	POPF				;Restore interrupt state
CMOS_RET:
	RET				;				   @RH4
ADJUST_CMOS	ENDP

