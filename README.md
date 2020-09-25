# FFXIV Rotation Simulator and AI based on Q-learning and Neural Networks created by Lady Yuna'lesca of Exodus

Shadowbringers BLM logs: https://drive.google.com/drive/folders/1bduP1IPUHhTlXDbwFBXJtyTG6pBBnrbw

Simulator details 

- Caster tax is applied to the end of casts. 

- Cast confirmation happens 0.5s before the end of a cast. 

- DoTs and buffs apply immediately.

- Instant actions (OGCDs and instant casts) are assumed to take 0.6s of animation lock + 0.1s of caster tax.

- The simulator cannot wait for arbitrary periods of time.

- Given a state and a set of actions, predicts the expected dps for the next X seconds for each action assuming that all subsequent actions follow a greedy policy.
