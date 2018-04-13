        PUBLIC meanfilter3

        SECTION .text : CODE (2)
        THUMB
        
        // Finalidade dos registradores
        // R0   - dim_x
        // R1   - dim_y => first_element_last_line = dim_x * (dim_y - 1)
        // R2   - img_in
        // R3   - img_out => pointer_out
        // R4   - accL0
        // R5   - accL1
        // R6   - accL2
        // R7   - c0
        // R8   - c1
        // R9   - c2
        // R10  - pointer
        // R11  - column_pointer
        // R12  - last_valid_pointer = dim_x * dim_y - 3
        
meanfilter3:
        push    {R4-R12,LR}
        
        // Cálculo do tamanho da imagem de saída
        sub     R7,R0,#2
        sub     R8,R1,#2
        mul     R9,R7,R8                        // tamanho = (dim_x - 2) * (dim_y - 2)
        push    {R9}                            // coloca o tamanho na pilha
                
        // Cálculo de last_valid_pointer (ponteiro que deve redirecionar para return => R12)
        // Cálculo de first_element_last_line (valor limite para redirecionar para nextColumn => R1)
        mul     R12,R0,R1                       // R12 = dim_x * dim_y
        sub     R1,R12,R0                       // (offset) first_element_last_line = {dim_x * dim_y} - dim_x = dim_x * (dim_y - 1)
        sub     R12,R12,#3                      // (offset) last_valid_pointer = {dim_x * dim_y} - 3
        add     R1,R1,R2                        // first_element_last_line
        add     R12,R12,R2                      // last_valid_pointer
        
        // Inicialização
        mov     R11,R2                          // column_pointer = img_in
        
nextColumn:
        // Empilha pointer_out da enésima coluna
        push    {R3}
        
        // Faz a conta (nova coluna)
        // Primeira linha (L0)
        mov     R10,R11                         // pointer = column_pointer
        ldrb    R7,[R10,#0]                     // coluna 0
        ldrb    R8,[R10,#1]                     // coluna 1
        ldrb    R9,[R10,#2]                     // coluna 2
        add     R4,R7,R8
        add     R4,R4,R9                        // accL0 = c0 + c1 + c2
        // Segunda linha (L1)
        add     R10,R10,R0                      // pointer += dim_x
        ldrb    R7,[R10,#0]                     // coluna 0
        ldrb    R8,[R10,#1]                     // coluna 1
        ldrb    R9,[R10,#2]                     // coluna 2
        add     R5,R7,R8
        add     R5,R5,R9                        // accL1 = c0 + c1 + c2
        // Terceira linha (L2)
        add     R10,R10,R0                      // pointer += dim_x
        ldrb    R7,[R10,#0]                     // coluna 0
        ldrb    R8,[R10,#1]                     // coluna 1
        ldrb    R9,[R10,#2]                     // coluna 2
        add     R6,R7,R8
        add     R6,R6,R9                        // accL2 = c0 + c1 + c2
        b       store

nextLine:        
        // Faz a conta (nova linha)
        mov     R4,R5                           // R4 = R5 (2ª linha anterior => 1ª linha)
        mov     R5,R6                           // R5 = R6 (3ª linha anterior => 2ª linha)
        // R6: nova linha
        add     R10,R10,R0                      // pointer += dim_x
        ldrb    R7,[R10,#0]                     // coluna 0
        ldrb    R8,[R10,#1]                     // coluna 1
        ldrb    R9,[R10,#2]                     // coluna 2
        add     R6,R7,R8
        add     R6,R6,R9                        // accL2 = c0 + c1 + c2
        
store:
        // Calcula (accL0 + accL1 + accL2)/9 e persiste
        // R7-R9 serão reescritos na próxima iteração, portanto posso utilizá-los
        add     R7,R4,R5
        add     R7,R7,R6                        // acc = accL0 + accL1 + accL2
        mov     R8,#9
        udiv    R7,R8                           // acc /= 9
        strb    R7,[R3,#0]                      // pointer_out = acc
        sub     R9,R0,#2
        add     R3,R3,R9                        // pointer_out += dim_x - 2
        
        // Checa se esta é a última média a ser calculada
        // se pointer (R10) == (R12) last_valid_pointer então branch para return
        cmp     R10,R12                         // compara pointer com last_valid_pointer
        beq     return                          // terminou de calcular
        
        // Checa se esta é a última linha a ser calculada
        // se pointer (R10) >= (R1) first_element_last_line então branch para nextColumn
        cmp     R10,R1                          // compara pointer com first_element_last_line
        ittt    cs
        addcs   R11,R11,#1                      // column_pointer += 1
        popcs   {R3}
        addcs   R3,R3,#1                        // seta pointer_out para a próxima coluna
        bcs     nextColumn                      // próxima coluna
        // se não, branch para nextLine
        b       nextLine                        // próxima linha

return:
        // Finalização da rotina
        pop     {R0,R1}
        mov     R0,R1                           // retira o tamanho da pilha
        pop     {R4-R12,LR}
        bx      LR

        END

