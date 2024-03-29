package ru.unit.morphine.assembly.compiler.ast.node

data class AccessAccessible(
    val container: Expression,
    val key: Expression,
    override val data: Node.Data
) : Accessible, Expression
