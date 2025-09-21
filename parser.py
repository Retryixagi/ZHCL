# -*- coding: utf-8 -*-
"""
語法分析器 (Parser)
負責將詞彙單元序列解析為抽象語法樹 (AST)
"""

from typing import List, Dict, Any, Optional
from enum import Enum
from lexer import Token, TokenType

class ASTNodeType(Enum):
    """AST 節點類型"""
    PROGRAM = "program"
    FUNCTION_DECLARATION = "function_declaration"
    VARIABLE_DECLARATION = "variable_declaration"
    STATEMENT = "statement"
    EXPRESSION = "expression"
    BINARY_EXPRESSION = "binary_expression"
    UNARY_EXPRESSION = "unary_expression"
    ASSIGNMENT_EXPRESSION = "assignment_expression"
    LITERAL = "literal"
    IDENTIFIER = "identifier"
    FUNCTION_CALL = "function_call"
    IF_STATEMENT = "if_statement"
    WHILE_STATEMENT = "while_statement"
    FOR_STATEMENT = "for_statement"
    DO_WHILE_STATEMENT = "do_while_statement"
    SWITCH_STATEMENT = "switch_statement"
    CASE_STATEMENT = "case_statement"
    DEFAULT_STATEMENT = "default_statement"
    BREAK_STATEMENT = "break_statement"
    CONTINUE_STATEMENT = "continue_statement"
    GOTO_STATEMENT = "goto_statement"
    RETURN_STATEMENT = "return_statement"
    BLOCK_STATEMENT = "block_statement"
    CAST_EXPRESSION = "cast_expression"
    SIZEOF_EXPRESSION = "sizeof_expression"
    CONDITIONAL_EXPRESSION = "conditional_expression"
    POSTFIX_EXPRESSION = "postfix_expression"

class ASTNode:
    """抽象語法樹節點"""

    def __init__(self, type_: ASTNodeType, **attributes):
        self.type = type_
        self.attributes = attributes
        self.children: List[ASTNode] = []

    def add_child(self, child: 'ASTNode'):
        """添加子節點"""
        self.children.append(child)

    def __repr__(self):
        return f"ASTNode({self.type}, {self.attributes})"

    def to_dict(self) -> Dict[str, Any]:
        """將 AST 節點轉換為字典"""
        return {
            'type': self.type.value,
            'attributes': self.attributes,
            'children': [child.to_dict() for child in self.children]
        }

class ParserError(Exception):
    """語法分析錯誤"""

    def __init__(self, message: str, token: Token = None):
        self.message = message
        self.token = token
        if token:
            super().__init__(f"{message} at line {token.line}, column {token.column}")
        else:
            super().__init__(message)

class Parser:
    """語法分析器"""

    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.position = 0
        self.current_token = self.tokens[0] if self.tokens else None

    def advance(self):
        """前進到下一個詞彙單元"""
        self.position += 1
        if self.position >= len(self.tokens):
            self.current_token = None
        else:
            self.current_token = self.tokens[self.position]

    def peek(self, offset: int = 1) -> Optional[Token]:
        """查看後面的詞彙單元"""
        peek_pos = self.position + offset
        if peek_pos >= len(self.tokens):
            return None
        return self.tokens[peek_pos]

    def expect(self, token_type: TokenType, value: str = None) -> Token:
        """期望特定的詞彙單元"""
        if not self.current_token:
            raise ParserError("Unexpected end of input")

        if self.current_token.type != token_type:
            raise ParserError(f"Expected {token_type}, got {self.current_token.type}",
                            self.current_token)

        if value and self.current_token.value != value:
            raise ParserError(f"Expected '{value}', got '{self.current_token.value}'",
                            self.current_token)

        token = self.current_token
        self.advance()
        return token

    def skip_preprocessor_directive(self):
        """跳過預處理器指令"""
        # 我們已經看到了 #，現在需要跳過整個指令

        # 跳過指令名稱（如 include）
        if self.current_token and self.current_token.type == TokenType.IDENTIFIER:
            self.advance()

        # 對於 include 指令，跳過 <...> 或 "..." 部分
        if self.current_token and self.current_token.type == TokenType.LESS:
            # 跳過 < 符號
            self.advance()
            # 跳過文件名部分
            while self.current_token and self.current_token.type not in [TokenType.GREATER, TokenType.SEMICOLON]:
                self.advance()
            # 跳過 > 符號
            if self.current_token and self.current_token.type == TokenType.GREATER:
                self.advance()

        elif self.current_token and self.current_token.type == TokenType.STRING_LITERAL:
            # 跳過 "..." 形式的頭文件
            self.advance()

        # 跳過其他可能的 tokens 直到我們到達下一個語句的開始
        # 我們通過檢查是否到達了函式或變數宣告的開始來判斷
        while self.current_token and self.current_token.type not in [
            TokenType.INT, TokenType.VOID, TokenType.CHAR, TokenType.FLOAT,
            TokenType.DOUBLE, TokenType.SHORT, TokenType.LONG, TokenType.UNSIGNED,
            TokenType.EOF
        ]:
            self.advance()

    def parse(self) -> ASTNode:
        """開始解析"""
        return self.parse_program()

    def parse_program(self) -> ASTNode:
        """解析程式"""
        program = ASTNode(ASTNodeType.PROGRAM)

        while self.current_token and self.current_token.type != TokenType.EOF:
            if self.current_token.type == TokenType.HASH:
                # 預處理器指令，跳過整行
                self.skip_preprocessor_directive()
                continue
            elif self.current_token.type in [TokenType.INT, TokenType.VOID, TokenType.CHAR,
                                         TokenType.FLOAT, TokenType.DOUBLE, TokenType.SHORT, TokenType.LONG, TokenType.UNSIGNED]:
                # 可能是函式或變數宣告
                if self.peek() and self.peek().type == TokenType.IDENTIFIER:
                    if self.peek(2) and self.peek(2).type == TokenType.LEFT_PAREN:
                        # 函式宣告
                        program.add_child(self.parse_function_declaration())
                    else:
                        # 全域變數宣告
                        program.add_child(self.parse_variable_declaration())
                else:
                    raise ParserError("Expected identifier after type", self.current_token)
            elif self.current_token.type == TokenType.UNKNOWN:
                # 跳過未知的編譯器特定符號
                self.advance()
                continue
            elif self.current_token.type == TokenType.SEMICOLON:
                # 空語句
                self.advance()
            else:
                raise ParserError(f"Unexpected token: {self.current_token.type}",
                                self.current_token)

        return program

    def parse_function_declaration(self) -> ASTNode:
        """解析函式宣告"""
        # 解析回傳型別
        return_type = self.parse_type_specifier()

        # 解析函式名稱
        function_name = self.expect(TokenType.IDENTIFIER).value

        # 解析參數列表
        self.expect(TokenType.LEFT_PAREN)
        parameters = self.parse_parameter_list()
        self.expect(TokenType.RIGHT_PAREN)

        # 解析函式主體
        body = self.parse_block_statement()

        return ASTNode(
            ASTNodeType.FUNCTION_DECLARATION,
            name=function_name,
            return_type=return_type,
            parameters=parameters,
            body=body
        )

    def parse_variable_declaration(self) -> ASTNode:
        """解析變數宣告"""
        var_type = self.parse_type_specifier()
        var_name = self.expect(TokenType.IDENTIFIER).value

        # 檢查是否有初始值
        initializer = None
        if self.current_token and self.current_token.type == TokenType.ASSIGN:
            self.advance()  # 跳過 =
            initializer = self.parse_expression()

        self.expect(TokenType.SEMICOLON)

        return ASTNode(
            ASTNodeType.VARIABLE_DECLARATION,
            name=var_name,
            var_type=var_type,
            initializer=initializer
        )

    def parse_type_specifier(self) -> str:
        """解析型別指定符"""
        type_tokens = []

        # 基本型別
        if self.current_token.type == TokenType.INT:
            type_tokens.append("int")
        elif self.current_token.type == TokenType.VOID:
            type_tokens.append("void")
        elif self.current_token.type == TokenType.CHAR:
            type_tokens.append("char")
        elif self.current_token.type == TokenType.FLOAT:
            type_tokens.append("float")
        elif self.current_token.type == TokenType.DOUBLE:
            type_tokens.append("double")
        elif self.current_token.type == TokenType.SHORT:
            type_tokens.append("short")
        elif self.current_token.type == TokenType.LONG:
            type_tokens.append("long")
        elif self.current_token.type == TokenType.UNSIGNED:
            type_tokens.append("unsigned")
        else:
            raise ParserError(f"Expected type specifier, got {self.current_token.type}",
                            self.current_token)

        self.advance()

        # 處理修飾符 (const, unsigned, etc.)
        while self.current_token and self.current_token.type in [
            TokenType.CONST, TokenType.UNSIGNED, TokenType.SIGNED,
            TokenType.STATIC, TokenType.EXTERN
        ]:
            type_tokens.insert(0, self.current_token.value)
            self.advance()

        # 處理指標修飾符
        while self.current_token and self.current_token.type == TokenType.MULTIPLY:
            type_tokens.append("*")
            self.advance()

        return " ".join(type_tokens)

    def parse_parameter_list(self) -> List[Dict[str, str]]:
        """解析參數列表"""
        parameters = []

        if self.current_token and self.current_token.type != TokenType.RIGHT_PAREN:
            while True:
                if self.current_token.type == TokenType.VOID:
                    # void 參數列表
                    self.advance()
                    break

                param_type = self.parse_type_specifier()
                param_name = self.expect(TokenType.IDENTIFIER).value

                parameters.append({
                    'name': param_name,
                    'type': param_type
                })

                if self.current_token and self.current_token.type == TokenType.COMMA:
                    self.advance()
                else:
                    break

        return parameters

    def parse_block_statement(self) -> ASTNode:
        """解析區塊語句"""
        self.expect(TokenType.LEFT_BRACE)

        block = ASTNode(ASTNodeType.BLOCK_STATEMENT)

        while self.current_token and self.current_token.type != TokenType.RIGHT_BRACE:
            if self.current_token.type == TokenType.SEMICOLON:
                # 空語句
                self.advance()
            else:
                statement = self.parse_statement()
                if statement:
                    block.add_child(statement)

        self.expect(TokenType.RIGHT_BRACE)

        return block

    def parse_statement(self) -> Optional[ASTNode]:
        """解析語句"""
        if not self.current_token:
            return None

        if self.current_token.type in [TokenType.INT, TokenType.VOID, TokenType.CHAR,
                                     TokenType.FLOAT, TokenType.DOUBLE]:
            # 區域變數宣告
            return self.parse_variable_declaration()

        elif self.current_token.type == TokenType.IDENTIFIER:
            # 可能是賦值語句、函式呼叫、後置運算符或獨立表達式
            identifier = self.current_token.value
            self.advance()

            if self.current_token and self.current_token.type in [
                TokenType.ASSIGN, TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN,
                TokenType.MULTIPLY_ASSIGN, TokenType.DIVIDE_ASSIGN, TokenType.MODULO_ASSIGN,
                TokenType.AND_ASSIGN, TokenType.OR_ASSIGN, TokenType.XOR_ASSIGN,
                TokenType.SHIFT_LEFT_ASSIGN, TokenType.SHIFT_RIGHT_ASSIGN
            ]:
                # 賦值語句
                operator = self.current_token.value
                self.advance()  # 跳過運算符
                expression = self.parse_expression()
                self.expect(TokenType.SEMICOLON)

                return ASTNode(
                    ASTNodeType.STATEMENT,
                    statement_type="assignment",
                    target=identifier,
                    operator=operator,
                    value=expression
                )
            elif self.current_token and self.current_token.type == TokenType.LEFT_PAREN:
                # 函式呼叫
                self.advance()  # 跳過 (
                args = self.parse_argument_list()
                self.expect(TokenType.RIGHT_PAREN)
                self.expect(TokenType.SEMICOLON)

                return ASTNode(
                    ASTNodeType.STATEMENT,
                    statement_type="function_call",
                    function_name=identifier,
                    arguments=args
                )
            elif self.current_token and self.current_token.type in [TokenType.INCREMENT, TokenType.DECREMENT]:
                # 後置遞增/遞減
                operator = self.current_token.value
                self.advance()
                self.expect(TokenType.SEMICOLON)

                return ASTNode(
                    ASTNodeType.STATEMENT,
                    statement_type="postfix_expression",
                    target=identifier,
                    operator=operator
                )
            else:
                raise ParserError("Expected assignment operator, '(', '++', '--' or other operator after identifier", self.current_token)

        elif self.current_token.type in [TokenType.INCREMENT, TokenType.DECREMENT]:
            # 前置遞增/遞減
            operator = self.current_token.value
            self.advance()
            identifier = self.expect(TokenType.IDENTIFIER).value
            self.expect(TokenType.SEMICOLON)

            return ASTNode(
                ASTNodeType.STATEMENT,
                statement_type="prefix_expression",
                target=identifier,
                operator=operator
            )

        elif self.current_token.type == TokenType.RETURN:
            # return 語句
            self.advance()
            expression = None
            if self.current_token and self.current_token.type != TokenType.SEMICOLON:
                expression = self.parse_expression()
            self.expect(TokenType.SEMICOLON)

            return ASTNode(
                ASTNodeType.RETURN_STATEMENT,
                expression=expression
            )

        elif self.current_token.type == TokenType.IF:
            # if 語句
            return self.parse_if_statement()

        elif self.current_token.type == TokenType.WHILE:
            # while 語句
            return self.parse_while_statement()

        elif self.current_token.type == TokenType.FOR:
            # for 語句
            return self.parse_for_statement()

        elif self.current_token.type == TokenType.DO:
            # do-while 語句
            return self.parse_do_while_statement()

        elif self.current_token.type == TokenType.SWITCH:
            # switch 語句
            return self.parse_switch_statement()

        elif self.current_token.type == TokenType.BREAK:
            # break 語句
            return self.parse_break_statement()

        elif self.current_token.type == TokenType.CONTINUE:
            # continue 語句
            return self.parse_continue_statement()

        elif self.current_token.type == TokenType.GOTO:
            # goto 語句
            return self.parse_goto_statement()

        elif self.current_token.type == TokenType.SEMICOLON:
            # 空語句
            self.advance()
            return ASTNode(ASTNodeType.STATEMENT, statement_type="empty")

        elif self.current_token.type == TokenType.LEFT_BRACE:
            # 區塊語句
            return self.parse_block_statement()

        else:
            raise ParserError(f"Unexpected token in statement: {self.current_token.type}",
                            self.current_token)

    def parse_if_statement(self) -> ASTNode:
        """解析 if 語句"""
        self.expect(TokenType.IF)
        self.expect(TokenType.LEFT_PAREN)
        condition = self.parse_expression()
        self.expect(TokenType.RIGHT_PAREN)

        then_branch = self.parse_statement()

        else_branch = None
        if self.current_token and self.current_token.type == TokenType.ELSE:
            self.advance()
            else_branch = self.parse_statement()

        return ASTNode(
            ASTNodeType.IF_STATEMENT,
            condition=condition,
            then_branch=then_branch,
            else_branch=else_branch
        )

    def parse_while_statement(self) -> ASTNode:
        """解析 while 語句"""
        self.expect(TokenType.WHILE)
        self.expect(TokenType.LEFT_PAREN)
        condition = self.parse_expression()
        self.expect(TokenType.RIGHT_PAREN)

        body = self.parse_statement()

        return ASTNode(
            ASTNodeType.WHILE_STATEMENT,
            condition=condition,
            body=body
        )

    def parse_for_statement(self) -> ASTNode:
        """解析 for 語句"""
        self.expect(TokenType.FOR)
        self.expect(TokenType.LEFT_PAREN)

        # 初始化
        initialization = None
        if self.current_token and self.current_token.type != TokenType.SEMICOLON:
            if self.current_token.type in [TokenType.INT, TokenType.VOID, TokenType.CHAR,
                                         TokenType.FLOAT, TokenType.DOUBLE, TokenType.SHORT, TokenType.LONG, TokenType.UNSIGNED]:
                # 變數聲明
                initialization = self.parse_variable_declaration()
                # parse_variable_declaration 已經消耗了分號，所以不需要再期望分號
            else:
                # 表達式
                initialization = self.parse_expression()
                self.expect(TokenType.SEMICOLON)
        else:
            # 空初始化
            self.expect(TokenType.SEMICOLON)

        # 條件
        condition = None
        if self.current_token and self.current_token.type != TokenType.SEMICOLON:
            condition = self.parse_expression()
        self.expect(TokenType.SEMICOLON)

        # 增量
        increment = None
        if self.current_token and self.current_token.type != TokenType.RIGHT_PAREN:
            increment = self.parse_expression()
        self.expect(TokenType.RIGHT_PAREN)

        body = self.parse_statement()

        return ASTNode(
            ASTNodeType.FOR_STATEMENT,
            initialization=initialization,
            condition=condition,
            increment=increment,
            body=body
        )

    def parse_do_while_statement(self) -> ASTNode:
        """解析 do-while 語句"""
        self.expect(TokenType.DO)
        body = self.parse_statement()
        self.expect(TokenType.WHILE)
        self.expect(TokenType.LEFT_PAREN)
        condition = self.parse_expression()
        self.expect(TokenType.RIGHT_PAREN)
        self.expect(TokenType.SEMICOLON)

        return ASTNode(
            ASTNodeType.DO_WHILE_STATEMENT,
            body=body,
            condition=condition
        )

    def parse_switch_statement(self) -> ASTNode:
        """解析 switch 語句"""
        self.expect(TokenType.SWITCH)
        self.expect(TokenType.LEFT_PAREN)
        expression = self.parse_expression()
        self.expect(TokenType.RIGHT_PAREN)
        self.expect(TokenType.LEFT_BRACE)

        cases = []
        default_case = None

        while self.current_token and self.current_token.type != TokenType.RIGHT_BRACE:
            if self.current_token.type == TokenType.CASE:
                case = self.parse_case_statement()
                cases.append(case)
            elif self.current_token.type == TokenType.DEFAULT:
                if default_case is not None:
                    raise ParserError("Multiple default cases in switch statement", self.current_token)
                default_case = self.parse_default_statement()
            else:
                raise ParserError(f"Unexpected token in switch statement: {self.current_token.type}",
                                self.current_token)

        self.expect(TokenType.RIGHT_BRACE)

        return ASTNode(
            ASTNodeType.SWITCH_STATEMENT,
            expression=expression,
            cases=cases,
            default_case=default_case
        )

    def parse_case_statement(self) -> ASTNode:
        """解析 case 語句"""
        self.expect(TokenType.CASE)
        value = self.parse_expression()
        self.expect(TokenType.COLON)

        statements = []
        while (self.current_token and
               self.current_token.type not in [TokenType.CASE, TokenType.DEFAULT, TokenType.RIGHT_BRACE]):
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)

        return ASTNode(
            ASTNodeType.CASE_STATEMENT,
            value=value,
            statements=statements
        )

    def parse_default_statement(self) -> ASTNode:
        """解析 default 語句"""
        self.expect(TokenType.DEFAULT)
        self.expect(TokenType.COLON)

        statements = []
        while (self.current_token and
               self.current_token.type not in [TokenType.CASE, TokenType.DEFAULT, TokenType.RIGHT_BRACE]):
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)

        return ASTNode(
            ASTNodeType.DEFAULT_STATEMENT,
            statements=statements
        )

    def parse_break_statement(self) -> ASTNode:
        """解析 break 語句"""
        self.expect(TokenType.BREAK)
        self.expect(TokenType.SEMICOLON)
        return ASTNode(ASTNodeType.BREAK_STATEMENT)

    def parse_continue_statement(self) -> ASTNode:
        """解析 continue 語句"""
        self.expect(TokenType.CONTINUE)
        self.expect(TokenType.SEMICOLON)
        return ASTNode(ASTNodeType.CONTINUE_STATEMENT)

    def parse_goto_statement(self) -> ASTNode:
        """解析 goto 語句"""
        self.expect(TokenType.GOTO)
        label = self.expect(TokenType.IDENTIFIER).value
        self.expect(TokenType.SEMICOLON)

        return ASTNode(
            ASTNodeType.GOTO_STATEMENT,
            label=label
        )

    def parse_expression(self) -> ASTNode:
        """解析表達式 (完整版，支援賦值和逗號運算子)"""
        left = self.parse_conditional_expression()

        # 處理賦值運算子
        if self.current_token and self.current_token.type in [
            TokenType.ASSIGN, TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN,
            TokenType.MULTIPLY_ASSIGN, TokenType.DIVIDE_ASSIGN, TokenType.MODULO_ASSIGN,
            TokenType.SHIFT_LEFT_ASSIGN, TokenType.SHIFT_RIGHT_ASSIGN,
            TokenType.AND_ASSIGN, TokenType.OR_ASSIGN, TokenType.XOR_ASSIGN
        ]:
            operator = self.current_token.value
            self.advance()  # 跳過賦值運算符
            right = self.parse_expression()  # 遞歸調用支援鏈式賦值

            return ASTNode(
                ASTNodeType.ASSIGNMENT_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        # 處理逗號運算子 (優先級最低)
        while self.current_token and self.current_token.type == TokenType.COMMA:
            self.advance()  # 跳過逗號
            right = self.parse_expression()  # 遞歸調用以支援賦值運算符

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=',',
                left=left,
                right=right
            )

        return left

    def parse_conditional_expression(self) -> ASTNode:
        """解析條件表達式 (三元運算符 ?:)"""
        # 先解析邏輯或表達式作為條件
        condition = self.parse_logical_or_expression()

        # 檢查是否有三元運算符
        if self.current_token and self.current_token.type == TokenType.QUESTION:
            self.advance()  # 跳過 ?

            # 解析 true 表達式 (避免無限遞歸，使用邏輯或表達式)
            true_expr = self.parse_logical_or_expression()

            # 必須有 :
            self.expect(TokenType.COLON)

            # 解析 false 表達式 (避免無限遞歸)
            false_expr = self.parse_conditional_expression()  # 遞歸調用以支援嵌套

            return ASTNode(
                ASTNodeType.CONDITIONAL_EXPRESSION,
                condition=condition,
                true_expression=true_expr,
                false_expression=false_expr
            )

        return condition

    def parse_logical_or_expression(self) -> ASTNode:
        """解析邏輯或表達式"""
        left = self.parse_logical_and_expression()

        while self.current_token and self.current_token.type == TokenType.OR:
            operator = self.current_token.value
            self.advance()
            right = self.parse_logical_and_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_logical_and_expression(self) -> ASTNode:
        """解析邏輯與表達式"""
        left = self.parse_bitwise_or_expression()

        while self.current_token and self.current_token.type == TokenType.AND:
            operator = self.current_token.value
            self.advance()
            right = self.parse_bitwise_or_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_bitwise_or_expression(self) -> ASTNode:
        """解析位元或表達式"""
        left = self.parse_bitwise_xor_expression()

        while self.current_token and self.current_token.type == TokenType.BIT_OR:
            operator = self.current_token.value
            self.advance()
            right = self.parse_bitwise_xor_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_bitwise_xor_expression(self) -> ASTNode:
        """解析位元異或表達式"""
        left = self.parse_bitwise_and_expression()

        while self.current_token and self.current_token.type == TokenType.BIT_XOR:
            operator = self.current_token.value
            self.advance()
            right = self.parse_bitwise_and_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_bitwise_and_expression(self) -> ASTNode:
        """解析位元與表達式"""
        left = self.parse_relational_expression()

        while self.current_token and self.current_token.type == TokenType.BIT_AND:
            operator = self.current_token.value
            self.advance()
            right = self.parse_relational_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_equality_expression(self) -> ASTNode:
        """解析相等表達式"""
        left = self.parse_relational_expression()

        while self.current_token and self.current_token.type in [TokenType.EQUAL, TokenType.NOT_EQUAL]:
            operator = self.current_token.value
            self.advance()
            right = self.parse_relational_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_relational_expression(self) -> ASTNode:
        """解析關係表達式"""
        left = self.parse_shift_expression()

        while self.current_token and self.current_token.type in [
            TokenType.LESS, TokenType.LESS_EQUAL,
            TokenType.GREATER, TokenType.GREATER_EQUAL
        ]:
            operator = self.current_token.value
            self.advance()
            right = self.parse_shift_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_shift_expression(self) -> ASTNode:
        """解析移位表達式"""
        left = self.parse_additive_expression()

        while self.current_token and self.current_token.type in [TokenType.SHIFT_LEFT, TokenType.SHIFT_RIGHT]:
            operator = self.current_token.value
            self.advance()
            right = self.parse_additive_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_additive_expression(self) -> ASTNode:
        """解析加法表達式"""
        left = self.parse_multiplicative_expression()

        while self.current_token and self.current_token.type in [TokenType.PLUS, TokenType.MINUS]:
            operator = self.current_token.value
            self.advance()
            right = self.parse_multiplicative_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_multiplicative_expression(self) -> ASTNode:
        """解析乘法表達式"""
        left = self.parse_unary_expression()

        while self.current_token and self.current_token.type in [TokenType.MULTIPLY, TokenType.DIVIDE, TokenType.MODULO]:
            operator = self.current_token.value
            self.advance()
            right = self.parse_unary_expression()

            left = ASTNode(
                ASTNodeType.BINARY_EXPRESSION,
                operator=operator,
                left=left,
                right=right
            )

        return left

    def parse_unary_expression(self) -> ASTNode:
        """解析一元表達式"""
        if not self.current_token:
            raise ParserError("Unexpected end of input")

        if self.current_token.type in [TokenType.PLUS, TokenType.MINUS, TokenType.NOT, TokenType.BIT_NOT,
                                      TokenType.MULTIPLY, TokenType.BIT_AND]:
            # 一元運算符 (包括指標運算子)
            operator = self.current_token.value
            self.advance()
            operand = self.parse_unary_expression()

            return ASTNode(
                ASTNodeType.UNARY_EXPRESSION,
                operator=operator,
                operand=operand
            )
            operator = self.current_token.value
            self.advance()
            operand = self.parse_unary_expression()

            return ASTNode(
                ASTNodeType.UNARY_EXPRESSION,
                operator=operator,
                operand=operand
            )

        elif self.current_token.type in [TokenType.INCREMENT, TokenType.DECREMENT]:
            # 前置遞增/遞減
            operator = self.current_token.value
            self.advance()
            operand = self.parse_unary_expression()

            return ASTNode(
                ASTNodeType.UNARY_EXPRESSION,
                operator=operator,
                operand=operand,
                is_prefix=True
            )

        else:
            # 後置表達式
            return self.parse_postfix_expression()

    def parse_postfix_expression(self) -> ASTNode:
        """解析後置表達式"""
        primary = self.parse_primary_expression()

        while self.current_token and self.current_token.type in [TokenType.INCREMENT, TokenType.DECREMENT]:
            operator = self.current_token.value
            self.advance()

            primary = ASTNode(
                ASTNodeType.POSTFIX_EXPRESSION,
                operator=operator,
                operand=primary
            )

        return primary

    def parse_primary_expression(self) -> ASTNode:
        """解析主要表達式"""
        if not self.current_token:
            raise ParserError("Unexpected end of input")

        if self.current_token.type == TokenType.INTEGER_LITERAL:
            value = int(self.current_token.value)
            self.advance()
            return ASTNode(ASTNodeType.LITERAL, value=value, literal_type="int")

        elif self.current_token.type == TokenType.FLOAT_LITERAL:
            value = float(self.current_token.value)
            self.advance()
            return ASTNode(ASTNodeType.LITERAL, value=value, literal_type="float")

        elif self.current_token.type == TokenType.STRING_LITERAL:
            value = self.current_token.value
            self.advance()
            return ASTNode(ASTNodeType.LITERAL, value=value, literal_type="string")

        elif self.current_token.type == TokenType.CHAR_LITERAL:
            value = self.current_token.value
            self.advance()
            return ASTNode(ASTNodeType.LITERAL, value=value, literal_type="char")

        elif self.current_token.type == TokenType.SIZEOF:
            # sizeof 運算符
            return self.parse_sizeof_expression()

        elif self.current_token.type == TokenType.LEFT_PAREN:
            # 可能是類型轉換或括號表達式
            self.advance()  # 跳過 (

            # 檢查是否是類型轉換
            if self.current_token and self.current_token.type in [
                TokenType.INT, TokenType.VOID, TokenType.CHAR, TokenType.FLOAT,
                TokenType.DOUBLE, TokenType.SHORT, TokenType.LONG, TokenType.UNSIGNED
            ]:
                # 類型轉換
                target_type = self.parse_type_specifier()
                self.expect(TokenType.RIGHT_PAREN)
                expression = self.parse_expression()

                return ASTNode(
                    ASTNodeType.CAST_EXPRESSION,
                    target_type=target_type,
                    expression=expression
                )
            else:
                # 普通括號表達式
                expression = self.parse_expression()
                self.expect(TokenType.RIGHT_PAREN)
                return expression

        elif self.current_token.type == TokenType.IDENTIFIER:
            identifier = self.current_token.value
            self.advance()

            # 檢查是否是函式呼叫
            if self.current_token and self.current_token.type == TokenType.LEFT_PAREN:
                self.advance()  # 跳過 (
                args = self.parse_argument_list()
                self.expect(TokenType.RIGHT_PAREN)

                return ASTNode(
                    ASTNodeType.FUNCTION_CALL,
                    function_name=identifier,
                    arguments=args
                )
            else:
                return ASTNode(ASTNodeType.IDENTIFIER, name=identifier)

        else:
            raise ParserError(f"Unexpected token in expression: {self.current_token.type}",
                            self.current_token)

    def parse_sizeof_expression(self) -> ASTNode:
        """解析 sizeof 表達式"""
        self.expect(TokenType.SIZEOF)
        self.expect(TokenType.LEFT_PAREN)

        if self.current_token and self.current_token.type in [
            TokenType.INT, TokenType.VOID, TokenType.CHAR, TokenType.FLOAT,
            TokenType.DOUBLE, TokenType.SHORT, TokenType.LONG, TokenType.UNSIGNED
        ]:
            # 嘗試解析為類型（包括複雜類型如 int[10]）
            try:
                target_type = self.parse_type_specifier()

                # 檢查是否是陣列類型
                array_dimensions = []
                while self.current_token and self.current_token.type == TokenType.LEFT_BRACKET:
                    self.advance()  # 跳過 [
                    if self.current_token and self.current_token.type == TokenType.INTEGER_LITERAL:
                        size = int(self.current_token.value)
                        array_dimensions.append(size)
                        self.advance()  # 跳過數字
                    else:
                        # 動態大小陣列
                        array_dimensions.append(None)
                    self.expect(TokenType.RIGHT_BRACKET)

                self.expect(TokenType.RIGHT_PAREN)

                return ASTNode(
                    ASTNodeType.SIZEOF_EXPRESSION,
                    target_type=target_type,
                    array_dimensions=array_dimensions
                )
            except ParserError:
                # 如果類型解析失敗，回退到表達式解析
                pass

        # sizeof(表達式)
        expression = self.parse_expression()
        self.expect(TokenType.RIGHT_PAREN)

        return ASTNode(
            ASTNodeType.SIZEOF_EXPRESSION,
            expression=expression
        )

    def parse_argument_list(self) -> List[ASTNode]:
        """解析引數列表"""
        arguments = []

        if self.current_token and self.current_token.type != TokenType.RIGHT_PAREN:
            while True:
                arg = self.parse_expression()
                arguments.append(arg)

                if self.current_token and self.current_token.type == TokenType.COMMA:
                    self.advance()
                else:
                    break

        return arguments

def parse_tokens(tokens: List[Token]) -> ASTNode:
    """將詞彙單元序列解析為 AST"""
    parser = Parser(tokens)
    return parser.parse()

if __name__ == "__main__":
    from lexer import tokenize_source

    # 測試語法分析器
    test_code = """
    int add(int a, int b) {
        return a + b;
    }

    int main() {
        int x = 5;
        int y = 10;
        int result = add(x, y);
        return result;
    }
    """

    tokens = tokenize_source(test_code)
    ast = parse_tokens(tokens)

    def print_ast(node: ASTNode, indent: int = 0):
        print("  " * indent + str(node))
        for child in node.children:
            print_ast(child, indent + 1)

    print("AST:")
    print_ast(ast)