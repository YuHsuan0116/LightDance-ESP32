#pragma once

#include "player.h"

class State {
  public:
    virtual ~State() = default;
    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void handleEvent(Event& event) = 0;
    virtual void update() = 0;

  protected:
    State() = default;
};

class ResetState: public State {
  public:
    static ResetState& getInstance();
    void enter() override;
    void exit() override;
    void handleEvent(Event& event) override;
    void update() override;
};
class ReadyState: public State {
  public:
    static ReadyState& getInstance();
    void enter() override;
    void exit() override;
    void handleEvent(Event& event) override;
    void update() override;
};
class PlayingState: public State {
  public:
    static PlayingState& getInstance();
    void enter() override;
    void exit() override;
    void handleEvent(Event& event) override;
    void update() override;
};
class PauseState: public State {
  public:
    static PauseState& getInstance();
    void enter() override;
    void exit() override;
    void handleEvent(Event& event) override;
    void update() override;
};
class TestState: public State {
  public:
    static TestState& getInstance();
    void enter() override;
    void exit() override;
    void handleEvent(Event& event) override;
    void update() override;
};