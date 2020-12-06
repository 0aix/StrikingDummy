# FFXIV Rotation Simulator and AI using Q-learning and Neural Networks

#### Simulator details 

- Caster tax is applied to the end of casts. 

- Cast confirmation happens 0.5s before the end of a cast. 

- DoTs and buffs apply immediately.

- Instant actions (OGCDs and instant casts) are assumed to take 0.6s of animation lock + 0.1s of caster tax.

- The simulator cannot wait for arbitrary periods of time.

- Given a state and a set of actions, predicts the expected dps for the next X seconds for each action assuming that all subsequent actions follow a greedy policy.

**Shadowbringers BLM AI logs 5.2:** https://drive.google.com/drive/folders/1bduP1IPUHhTlXDbwFBXJtyTG6pBBnrbw

**Shadowbringers BLM AI logs 5.0:** https://drive.google.com/drive/folders/1xDV_06LRahb4lgCszoOrpr0bkAZo6UTA

**Stormblood BLM AI logs 4.5:** https://drive.google.com/drive/folders/1L-nuJQdjRamiJU2Sp-73dLyWna5WAYND

**Created by Lady Yuna'lesca (Fang0r#1047)**
