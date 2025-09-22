// AUTO-GENERATED or hand-written: include/zh_matrix.inc
#pragma once
// kind 定義需與 zh_symbols.inc 對齊：例如 0=WORD,1=OP,2=PUNCT,3=LIT
static constexpr float ZH_KIND_TRANS[4][4] = {
/*from\to  WORD   OP     PUNCT  LIT */
            { 0.2f, 0.6f, 0.1f,  0.1f },  // from WORD
            { 0.6f, 0.2f, 0.1f,  0.1f },  // from OP
            { 0.4f, 0.4f, 0.1f,  0.1f },  // from PUNCT
            { 0.5f, 0.3f, 0.1f,  0.1f },  // from LIT
};
static constexpr float ZH_LEN_BONUS = 0.02f;   // 每字元加分
static constexpr float ZH_TAG_W     = 0.30f;   // tag 相符權重
