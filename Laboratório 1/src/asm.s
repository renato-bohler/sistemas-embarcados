        PUBLIC meanfilter3

        SECTION .text : CODE (2)
        THUMB
        
meanfilter3:
        sub     R12,R0,#2               // dim_x-2 (out_x)
        sub     R6,R1,#2                // dim_y-2
        lsl     R8,R0,#1                // dim_x*2 (dbl_x)
        mul     R9,R0,R6    
        sub     R9,R9,#2                // dim_x*(dim_y - 2) - 2    (max_i)
        
        mov     R10,#0                  // i = 0
        mov     R6,#0                   // j = 0
loop:
        sub     R5,R9,R10
        cbz     R5, fim                 // sai do loop se i == max_i
        udiv    R5, R10, R0
        mls     R5, R0, R5, R10         // R5 = i % dim_x
        cmp     R5, R12                 // compara (i % dim_x) com out_x
        bcs     dont                    // pula para dont se (i % dim_x >= out_x)
        mov     R5,R10                  // cria um i temporario
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i]
        add     R5,R5,#1                // i+1
        mov     R4,R7                   // acc += img_in[i]
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i+1]
        add     R5,R5,#1                // i+2
        add     R4,R4,R7                // acc += img_in[i+1]
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i+2]
        add     R4,R4,R7                // acc += img_in[i+2]
        mov     R5,R10                  // i
        add     R5,R5,R0                // i+dim_x
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i+dim_x]
        add     R5,R5,#1                // i+dim_x+1
        add     R4,R4,R7                // acc += img_in[i+dim_x]
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i+dim_x+1]
        add     R5,R5,#1                // i+dim_x+2
        add     R4,R4,R7                // acc += img_in[i+dim_x+1]
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i+dim_x+2]
        add     R4,R4,R7                // acc += img_in[i+dim_x+2]
        mov     R5,R10                  // i
        add     R5,R5,R8                // i+dbl_x
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i+dbl_x]
        add     R5,R5,#1                // i+dbl_x+1
        add     R4,R4,R7                // acc += img_in[i+dbl_x]
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i+dbl_x+1]
        add     R5,R5,#1                // i+dbl_x+2
        add     R4,R4,R7                // acc += img_in[i+dbl_x+1]
        ldrb    R7,[R2,R5]              // coloca em r7 img_in[i+dbl_x+2]
        add     R4,R4,R7                // acc += img_in[i+dbl_x+1]
        mov     R5,#9                   // coloca no i_tmp o divisor, já que ele nao é mais usado até o fim dessa iteração
        udiv    R4,R4,R5                // acc/9
        strb    R4,[R3,R6]              // img_out[i]=(acc/9)       
        add     R6,R6,#1                // j++
dont:
        add     R10,R10,#1              // i++
        b       loop
fim:
        sub     R6,R1,#2                // dim_y-2 (repetindo pq faltou registrador)
        mul     R7,R12,R6               // tam = (dim_x - 2)*(dim_y - 2)
        mov     R0,R7                   // coloca o tamanho de img_out no retorno(R0)

        bx      lr

        END

