#pragma once

class StateMachine;

class State {
 public:
  virtual ~State() = default;
  virtual void enter();
  virtual void loop();
  virtual void exit();

  void setMachine(StateMachine* m) { machine = m; }

 protected:
  StateMachine* machine = nullptr;
};

class SearchingState : public State {
 private:
  int savedTagsTotal;

 public:
  void enter() override;
  void loop() override;
  void exit() override;
};

class SuccessState : public State {
 public:
  void enter() override;
  void loop() override;
  void exit() override;
};

class FailState : public State {
 public:
  void enter() override;
  void loop() override;
  void exit() override;
};

class StateMachine {
 private:
  State* current;

 public:
  StateMachine();
  void begin(State* initial);
  void change(State* next);
  void loop();
};
