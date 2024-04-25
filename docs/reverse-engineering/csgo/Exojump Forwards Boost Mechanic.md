## CSGO's Exojump Mechanic: __Forwards/Horizontal Boost__

This document is **not** about the Exojump's upwards boost mechanic.

### Introduction

A player equipped with an exojump can propel themselves forwards, backwards or sideways by crouch-jumping.
The boost amount is adjustable through the ConVar `sv_exojump_jumpbonus_forward`.

### Observations from testing

- Boost activation:
    - A player receives the boost if they...
        - have the exojump equipped
        - are on the ground
        - are in the process of crouching
        - jump, while pressing one or more directional movement keys (WASD)

- Boost manifestation:
    - The boost is a one-time horizontal (X and Y) velocity increase at the moment the jump key was pressed.

- Boost direction:
    - The player only receives a boost in the direction(s) of the movement key(s) that was/were pressed during the crouch-jump. Some examples:
        - No. 1: The player is running diagonally, pressing the forwards and the right key. The two keys stay pressed while crouch-jumping.
            - Result: The player receives a boost in the forwards key's and right key's directions.
        - No. 2: The player is running to the right. Right before crouch-jumping, the player lets go of the right key and starts pressing the forwards key.
            - Result: The player receives a boost in the forwards key's direction, but none in the right key's direction.

- Boost amount:
    - The boost amount does not depend on gravity.
    - The boost amount is usually constant for a given player equipment.
        - This assumes that the player is duck-jumping perfectly (pressing duck and jump simultaneously).
    - There is a maximum horizontal velocity for a given player equipment that the boost amount cannot overcome.
    - The further the ducking progress at the time of jumping, the less the boost amount.
    - Diagonally crouch-jumping with exo (holding W+A or W+D) sometimes results in ~3% more horizontal speed than doing it non-diagonally (holding just W).
    - The ConVar `sv_exojump_jumpbonus_forward` proportionally adjusts the boost amount (in the cases where the boost doesn't surpass the max horizontal velocity).
        - The max horizontal velocity amount grows with this ConVar too, but not proportionally.
