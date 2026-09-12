---
title: Additional seed entropy from dice rolls
nav_title: Additional seed entropy from dice rolls
category: advanced
---

Dice or coin flips can be mixed into the randomness used to build a Polyseed, on top of whatever your operating system supplies.

**When this is worth doing**

- Creating a seed on a live system or in a virtual machine, where the entropy pool may be thin
- You would rather not stake everything on your operating system's random number generator being sound

**How the mixing works**

Polyseed needs 19 bytes — 152 bits — of entropy. The tool sizes the number of throws as though your system contributed nothing at all, then combines your throws with system entropy anyway. That way a mistake on your part cannot make the result worse than the system alone.

**Do not write down your rolls.** They cannot reproduce the seed, so recording them gains you nothing and gives an attacker a starting point.

## Using it

Select **Create a new wallet** on the main menu, then press **Ctrl + K** on the next page and choose dice or coins.

### Dice

Tell the tool how many sides your die has. More sides means fewer throws:

| Die | Throws needed |
|-----|---------------|
| D6 | 59 |
| D12 | 42 |
| D20 | 36 |

The die has to be numbered 1 through N. Dice that skip values (2, 4, 6, 8, 10, 12) or that include a zero will not do.

Rolling several at once is fine as long as **every die has the same number of sides** — never mix a D6 with a D20.

With multiple dice, read the throwing area left to right and enter them in that order. Do not sort them, and do not re-roll a result because it looks insufficiently random; that bias is exactly what you are trying to avoid.

For a throw that lands like this:

```
⚃       ⚄
    ⚁
             ⚀
```

enter `4 2 5 1` and press **Next roll**.

Only ever enter real outcomes. Numbers you thought up are not random.

Keep going until **Rolls left** reaches zero, then press **Create polyseed**.

### Coins

Flip, then press **Heads** or **Tails** to match. Continue until **Flips left** reaches zero and press **Create polyseed**.

---

The generated Polyseed drops straight into the wallet creation wizard. **Write it down before continuing.**
