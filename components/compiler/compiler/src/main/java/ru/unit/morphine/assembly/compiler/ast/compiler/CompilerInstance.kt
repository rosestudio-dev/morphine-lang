package ru.unit.morphine.assembly.compiler.ast.compiler

import ru.unit.morphine.assembly.bytecode.Argument
import ru.unit.morphine.assembly.bytecode.Instruction
import ru.unit.morphine.assembly.bytecode.Value
import ru.unit.morphine.assembly.compiler.ast.compiler.exception.CompilerException
import ru.unit.morphine.assembly.compiler.ast.node.*
import java.util.*

class CompilerInstance(optimize: Boolean) : AbstractCompiler(optimize) {

    override fun eval(expression: Expression) = when (expression) {
        is AccessAccessible -> expression.eval()
        is VariableAccessible -> expression.eval()
        is BinaryExpression -> expression.eval()
        is BlockStatement -> expression.eval()
        is CallExpression -> expression.eval()
        is CallSelfExpression -> expression.eval()
        is EnvExpression -> expression.eval()
        is FunctionExpression -> expression.eval()
        is IfStatement -> expression.eval()
        is IncDecExpression -> expression.eval()
        is SelfExpression -> expression.eval()
        is TableExpression -> expression.eval()
        is UnaryExpression -> expression.eval()
        is ValueExpression -> expression.eval()
    }

    override fun set(accessible: Accessible, slot: Argument.Slot) = when (accessible) {
        is AccessAccessible -> accessible.set(slot)
        is VariableAccessible -> accessible.set(slot)
    }

    override fun exec(statement: Statement) = when (statement) {
        is AssigmentStatement -> statement.exec()
        is BlockStatement -> statement.exec()
        is BreakStatement -> statement.exec()
        is ContinueStatement -> statement.exec()
        is DeclarationStatement -> statement.exec()
        is DoWhileStatement -> statement.exec()
        is EmptyStatement -> Unit
        is EvalStatement -> statement.exec()
        is ForStatement -> statement.exec()
        is IfStatement -> statement.exec()
        is ReturnStatement -> statement.exec()
        is WhileStatement -> statement.exec()
        is YieldStatement -> statement.exec()
    }

    private fun ForStatement.exec() = codegenStatement {
        enterBreakContinue()

        function.variables.enter()

        initial.exec()

        anchor(function.continueAnchor)
        val condition = condition.evalWithResult()
        val blockAnchorUUID = UUID.randomUUID()

        instruction(
            Instruction.JumpIf(
                source = condition,
                ifPosition = positionByAnchor(blockAnchorUUID),
                elsePosition = positionByAnchor(function.breakAnchor)
            )
        )

        anchor(blockAnchorUUID)
        statement.exec()

        iterator.exec()

        instruction(
            Instruction.Jump(
                position = positionByAnchor(function.continueAnchor)
            )
        )

        function.variables.exit()

        anchor(function.breakAnchor)

        exitBreakContinue()
    }

    private fun WhileStatement.exec() = codegenStatement {
        enterBreakContinue()

        anchor(function.continueAnchor)
        val condition = condition.evalWithResult()
        val blockAnchorUUID = UUID.randomUUID()

        instruction(
            Instruction.JumpIf(
                source = condition,
                ifPosition = positionByAnchor(blockAnchorUUID),
                elsePosition = positionByAnchor(function.breakAnchor)
            )
        )

        function.variables.enter()

        anchor(blockAnchorUUID)
        statement.exec()

        instruction(
            Instruction.Jump(
                position = positionByAnchor(function.continueAnchor)
            )
        )

        function.variables.exit()

        anchor(function.breakAnchor)

        exitBreakContinue()
    }

    private fun DoWhileStatement.exec() = codegenStatement {
        enterBreakContinue()

        anchor(function.continueAnchor)

        function.variables.enter()

        statement.exec()

        val condition = condition.evalWithResult()

        instruction(
            Instruction.JumpIf(
                source = condition,
                ifPosition = positionByAnchor(function.continueAnchor),
                elsePosition = positionByAnchor(function.breakAnchor)
            )
        )

        function.variables.exit()

        anchor(function.breakAnchor)

        exitBreakContinue()
    }

    private fun BreakStatement.exec() = codegenStatement {
        instruction(
            Instruction.Jump(
                position = positionByAnchor(function.breakAnchor)
            )
        )
    }

    private fun ContinueStatement.exec() = codegenStatement {
        instruction(
            Instruction.Jump(
                position = positionByAnchor(function.continueAnchor)
            )
        )
    }

    private fun IfStatement.eval() = codegenExpression {
        val condition = condition.evalWithResult()
        val ifAnchorUUID = UUID.randomUUID()
        val elseAnchorUUID = UUID.randomUUID()
        val endAnchorUUID = UUID.randomUUID()

        instruction(
            Instruction.JumpIf(
                source = condition,
                ifPosition = positionByAnchor(ifAnchorUUID),
                elsePosition = positionByAnchor(elseAnchorUUID),
            )
        )

        fun produce(slot: Argument.Slot) {
            anchor(ifAnchorUUID)
            (ifStatement as? Expression)?.evalWithResult(slot) ?: throw CompilerException("Expected expression")
            instruction(
                Instruction.Jump(
                    position = positionByAnchor(endAnchorUUID)
                )
            )

            anchor(elseAnchorUUID)
            (elseStatement as? Expression)?.evalWithResult(slot) ?: throw CompilerException("Expected expression")

            anchor(endAnchorUUID)
        }

        result(
            withResult = { resultSlot ->
                produce(resultSlot)
            },
            withoutResult = {
                produce(slot())
            }
        )
    }

    private fun IfStatement.exec() = codegenStatement {
        val condition = condition.evalWithResult()
        val ifAnchorUUID = UUID.randomUUID()
        val elseAnchorUUID = UUID.randomUUID()
        val endAnchorUUID = UUID.randomUUID()

        instruction(
            Instruction.JumpIf(
                source = condition,
                ifPosition = positionByAnchor(ifAnchorUUID),
                elsePosition = positionByAnchor(elseAnchorUUID),
            )
        )

        anchor(ifAnchorUUID)
        ifStatement.exec()
        instruction(
            Instruction.Jump(
                position = positionByAnchor(endAnchorUUID)
            )
        )

        anchor(elseAnchorUUID)
        elseStatement.exec()

        anchor(endAnchorUUID)
    }

    private fun IncDecExpression.eval() = codegenExpression {
        val value = accessible.evalWithResult()

        fun produce(slot: Argument.Slot) {
            instruction(
                Instruction.Load(
                    constant = constant(Value.Integer(1)),
                    destination = slot
                ),
                when (type) {
                    IncDecExpression.Type.INCREMENT -> Instruction.Add(
                        sourceLeft = value,
                        sourceRight = slot,
                        destination = slot
                    )

                    IncDecExpression.Type.DECREMENT -> Instruction.Sub(
                        sourceLeft = value,
                        sourceRight = slot,
                        destination = slot
                    )
                }
            )

            accessible.set(slot)
        }

        result(
            withResult = { resultSlot ->
                produce(resultSlot)

                if (isPostfix) {
                    instruction(
                        Instruction.Move(
                            source = value,
                            destination = resultSlot
                        )
                    )
                }
            },
            withoutResult = {
                produce(slot())
            }
        )
    }

    private fun ReturnStatement.exec() = codegenStatement {
        instruction(
            Instruction.Leave(
                source = expression.evalWithResult()
            )
        )
    }

    private fun YieldStatement.exec() = codegenStatement {
        instruction(Instruction.Yield())
    }

    private fun AssigmentStatement.exec() = codegenStatement {
        when {
            accessibles.isEmpty() || expressions.isEmpty() -> {
                throw CompilerException("Empty assigment")
            }

            accessibles.size == expressions.size -> {
                accessibles.forEachIndexed { index, accessible ->
                    val expression = expressions[index]

                    val source = if (binaryType != null) {
                        BinaryExpression(
                            type = binaryType,
                            expressionA = accessible,
                            expressionB = expression,
                            data = data
                        )
                    } else {
                        expression
                    }.evalWithResult()

                    accessible.set(source)
                }
            }

            expressions.size == 1 -> {
                if (binaryType != null) {
                    throw CompilerException("Decomposable assigment doesn't support binary operations")
                }

                val source = expressions.single().evalWithResult()
                val slot = slot()

                accessibles.forEachIndexed { index, accessible ->
                    instruction(
                        Instruction.Load(
                            constant = constant(Value.Integer(index)),
                            destination = slot
                        ),
                        Instruction.Get(
                            container = source,
                            keySource = slot,
                            destination = slot
                        )
                    )

                    accessible.set(slot)
                }
            }

            else -> {
                throw CompilerException("Incorrect assigment")
            }
        }
    }

    private fun DeclarationStatement.exec() = codegenStatement {
        when {
            names.isEmpty() || expressions.isEmpty() -> {
                throw CompilerException("Empty declaration")
            }

            names.size == expressions.size -> {
                names.forEachIndexed { index, name ->
                    val slot = expressions[index].evalWithResult()

                    val variable = defineVariable(
                        name = name,
                        isConst = !isMutable
                    )

                    instruction(
                        variableSet(
                            name = name,
                            variable = variable,
                            slot = slot
                        )
                    )
                }
            }

            expressions.size == 1 -> {
                val source = expressions.single().evalWithResult()
                val slot = slot()

                names.forEachIndexed { index, name ->
                    instruction(
                        Instruction.Load(
                            constant = constant(Value.Integer(index)),
                            destination = slot
                        ),
                        Instruction.Get(
                            container = source,
                            keySource = slot,
                            destination = slot
                        )
                    )

                    val variable = defineVariable(
                        name = name,
                        isConst = !isMutable,
                    )

                    instruction(
                        variableSet(
                            name = name,
                            variable = variable,
                            slot = slot
                        )
                    )
                }
            }

            else -> {
                throw CompilerException("Incorrect declaration")
            }
        }
    }

    private fun AccessAccessible.eval() = codegenExpression {
        val container = container.evalWithResult()
        val key = key.evalWithResult()

        fun produce(slot: Argument.Slot) = Instruction.Get(
            container = container,
            keySource = key,
            destination = slot
        )

        result(
            withResult = { resultSlot ->
                instruction(produce(resultSlot))
            },
            withoutResult = {
                instruction(produce(slot()))
            }
        )
    }

    private fun AccessAccessible.set(slot: Argument.Slot) = codegenAccessible {
        val container = container.evalWithResult()
        val key = key.evalWithResult()

        instruction(
            Instruction.Set(
                source = slot,
                keySource = key,
                container = container
            )
        )
    }

    private fun CallExpression.eval() = codegenExpression {
        val callable = expression.evalWithResult()

        val slots = arguments.map { argument ->
            argument.evalWithResult()
        }

        slots.forEachIndexed { index, slot ->
            instruction(
                Instruction.Param(
                    source = slot,
                    param = Argument.Param(index)
                )
            )
        }

        instruction(
            Instruction.Call(
                source = callable,
                count = Argument.Count(arguments.size)
            )
        )

        result { resultSlot ->
            instruction(
                Instruction.Result(
                    destination = resultSlot,
                )
            )
        }
    }

    private fun CallSelfExpression.eval() = codegenExpression {
        val self = self.evalWithResult()
        val callable = callable.evalWithResult()

        if (extractCallable) {
            instruction(
                Instruction.Get(
                    container = self,
                    keySource = callable,
                    destination = callable
                )
            )
        }

        val slots = arguments.map { argument ->
            argument.evalWithResult()
        }

        slots.forEachIndexed { index, slot ->
            instruction(
                Instruction.Param(
                    source = slot,
                    param = Argument.Param(index)
                )
            )
        }

        instruction(
            Instruction.SCall(
                source = callable,
                selfSource = self,
                count = Argument.Count(arguments.size)
            )
        )

        result { resultSlot ->
            instruction(
                Instruction.Result(
                    destination = resultSlot,
                )
            )
        }
    }

    private fun EnvExpression.eval() = codegenExpression {
        result { resultSlot ->
            instruction(Instruction.Environment(resultSlot))
        }
    }

    private fun SelfExpression.eval() = codegenExpression {
        result { resultSlot ->
            instruction(Instruction.Self(resultSlot))
        }
    }

    private fun TableExpression.eval() = codegenExpression {
        result(
            withResult = { resultSlot ->
                instruction(Instruction.Table(resultSlot))

                val keySlot = slot()
                val valueSlot = slot()
                elements.forEach { (key, value) ->
                    key.evalWithResult(keySlot)
                    value.evalWithResult(valueSlot)

                    instruction(
                        Instruction.Set(
                            source = valueSlot,
                            keySource = keySlot,
                            container = resultSlot
                        )
                    )
                }
            },
            withoutResult = {
                elements.forEach { (key, value) ->
                    key.eval()
                    value.eval()
                }
            }
        )
    }

    private fun UnaryExpression.eval() = codegenExpression {
        val a = expression.evalWithResult()

        fun produce(slot: Argument.Slot) = when (type) {
            UnaryExpression.Type.NEGATE -> Instruction.Negative(
                source = a,
                destination = slot
            )

            UnaryExpression.Type.NOT -> Instruction.Not(
                source = a,
                destination = slot
            )

            UnaryExpression.Type.TYPE -> Instruction.Type(
                source = a,
                destination = slot
            )

            UnaryExpression.Type.LEN -> Instruction.Length(
                source = a,
                destination = slot
            )

            UnaryExpression.Type.REF -> Instruction.Ref(
                source = a,
                destination = slot
            )

            UnaryExpression.Type.DEREF -> Instruction.Deref(
                source = a,
                destination = slot
            )
        }

        result(
            withoutResult = {
                instruction(produce(slot()))
            },
            withResult = { resultSlot ->
                instruction(produce(resultSlot))
            }
        )
    }

    private fun BinaryExpression.eval() = codegenExpression {
        val a = expressionA.evalWithResult()
        val b = expressionB.evalWithResult()

        fun produce(slot: Argument.Slot) = when (type) {
            BinaryExpression.Type.ADD -> Instruction.Add(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.SUB -> Instruction.Sub(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.MUL -> Instruction.Mul(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.DIV -> Instruction.Div(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.MOD -> Instruction.Mod(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.EQUALS -> Instruction.Equal(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.LESS -> Instruction.Less(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.LESS_EQUALS -> Instruction.LessEqual(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.OR -> Instruction.Or(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.AND -> Instruction.And(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )

            BinaryExpression.Type.CONCAT -> Instruction.Concat(
                sourceLeft = a,
                sourceRight = b,
                destination = slot
            )
        }

        result(
            withoutResult = {
                instruction(produce(slot()))
            },
            withResult = { resultSlot ->
                instruction(produce(resultSlot))
            }
        )
    }

    private fun EvalStatement.exec() = codegenStatement {
        expression.eval()
    }

    private fun BlockStatement.exec() = codegenStatement {
        function.variables.enter()

        statements.forEach { statement ->
            statement.exec()
        }

        function.variables.exit()
    }

    private fun BlockStatement.eval() = codegenExpression {
        function.variables.enter()

        val expressionStatement = statements.lastOrNull()
            ?: throw CompilerException("Expression block is empty")

        val expression = (expressionStatement as? Expression)
            ?: (expressionStatement as? EvalStatement)?.expression
            ?: throw CompilerException("Expression is expected at the end")

        statements.dropLast(1).forEach { statement ->
            statement.exec()
        }

        result(
            withResult = { resultSlot ->
                expression.evalWithResult(resultSlot)
            },
            withoutResult = {
                expression.eval()
            }
        )

        function.variables.exit()
    }

    private fun FunctionExpression.eval() = codegenExpression {
        functionEnter(
            name = name,
            arguments = arguments,
            statics = statics
        )

        function.temporaries.enter()

        val argumentSlot = slot()

        arguments.forEachIndexed { index, argument ->
            val access = defineVariable(argument, isConst = true)

            instruction(
                Instruction.Arg(
                    arg = Argument.Arg(index),
                    destination = argumentSlot
                ),
                variableSet(
                    name = argument,
                    variable = access,
                    slot = argumentSlot
                )
            )
        }

        if (name != null) {
            val access = defineVariable(name, isConst = true)

            instruction(
                Instruction.Recursion(
                    destination = argumentSlot
                ),
                variableSet(
                    name = name,
                    variable = access,
                    slot = argumentSlot
                )
            )
        }

        function.temporaries.exit()

        function.variables.enter()

        statement.exec()

        function.variables.exit()

        val info = functionExit()

        result { resultSlot ->
            instruction(
                Instruction.Load(
                    constant = constant(Value.Function(info.uuid)),
                    destination = resultSlot
                )
            )

            if (info.closures.isNotEmpty()) {
                val slots = info.closures.map { slot() }

                info.closures.mapIndexed { index, name ->
                    val slot = VariableAccessible(
                        name = name,
                        data = data
                    ).evalWithResult(slots[index])

                    instruction(
                        Instruction.Param(
                            param = Argument.Param(index),
                            source = slot
                        )
                    )
                }

                instruction(
                    Instruction.Closure(
                        source = resultSlot,
                        count = Argument.Count(info.closures.size),
                        destination = resultSlot
                    ),
                )
            }
        }
    }

    private fun ValueExpression.eval() = codegenExpression {
        result { resultSlot ->
            instruction(
                Instruction.Load(
                    constant = constant(value),
                    destination = resultSlot
                )
            )
        }
    }

    private fun VariableAccessible.eval() = codegenExpression {
        val variable = accessVariable(name)

        result { resultSlot ->
            val instruction = when (variable) {
                is Access.Argument -> Instruction.Arg(
                    arg = Argument.Arg(variable.index),
                    destination = resultSlot
                )

                is Access.Closure -> Instruction.GetClosure(
                    closure = Argument.Closure(variable.index),
                    destination = resultSlot
                )

                is Access.Recursion -> Instruction.Recursion(
                    destination = resultSlot
                )

                is Access.Static -> Instruction.GetStatic(
                    static = Argument.Static(variable.index),
                    destination = resultSlot
                )

                is Access.Variable -> Instruction.Move(
                    source = Argument.Slot(variable.index),
                    destination = resultSlot
                )
            }

            instruction(instruction)
        }
    }

    private fun VariableAccessible.set(slot: Argument.Slot) = codegenAccessible {
        val variable = accessVariable(name)

        instruction(
            variableSet(
                name = name,
                variable = variable,
                slot = slot
            )
        )
    }

    private fun variableSet(
        name: String,
        variable: Access,
        slot: Argument.Slot
    ) = when (variable) {
        is Access.Closure -> Instruction.SetClosure(
            closure = Argument.Closure(variable.index),
            source = slot
        )

        is Access.Static -> Instruction.SetStatic(
            static = Argument.Static(variable.index),
            source = slot
        )

        is Access.Variable -> Instruction.Move(
            source = slot,
            destination = Argument.Slot(variable.index)
        )

        is Access.Argument -> throw CompilerException("Argument '$name' cannot be modified")

        is Access.Recursion -> throw CompilerException("Recursion '$name' cannot be modified")
    }
}