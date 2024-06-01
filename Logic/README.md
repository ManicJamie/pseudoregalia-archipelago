# Logic

Logic is separated into Entrance logic & location logic. Both are YAMLs that are used to generate code.

Logical requirements are given as a list of lists: this should be considered as DNF, or [a list of `(list of requirements ANDed)` ORed].

All caps (eg. `ULTRA`) are decomposed required skips, abilities are alone but may have `.n` appended, where `n` is a count. `n` will be assumed as 1 if absent for an ability that expects a count.

If a clause is `null`, it will always be satisfied and hence the rule will always be satisfied.

## Updating

After updating the YAML, you must run `Generate.py`, which will replace the generated logic in `apworld/GeneratedRules.py`.

## Definitions

### Abilities

- Kick.n
- Kickflip.n: Kicks where 1 may be replaced with a Sunsetter flip.
- Slide
- Wind
- Bounce
- Navigate_Dark: Light OR Breaker w/ darkrooms on (future)

Future:

- Reverse_Kick: EXPERT or LUNATIC? Separate setting? (future)

### Settings

- ULTRA
- FULL_GOLD_PATCH: (future)
- SLIDE_GOLD_ULTRA: Specific to full-gold patch. (future)
- GOLD_ULTRA: Wind on LP, Slide on full-gold, ULTRA both.
- OBSCURE
- HARD (TODO: decompose)
- EXPERT (TODO: decompose)
- LUNATIC (TODO: decompose)
