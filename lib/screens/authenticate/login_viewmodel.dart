import 'package:boilerstat/services/firebase_manager.dart';
import 'package:firebase_auth/firebase_auth.dart';

import '../../viewmodel.dart';
import 'package:rxdart/rxdart.dart';

class LoginViewModel extends ViewModel {
  final BehaviorSubject<String> _email = BehaviorSubject<String>();
  final BehaviorSubject<String> _password = BehaviorSubject();
  final BehaviorSubject<bool> _button = BehaviorSubject();

  final BehaviorSubject<String> _dataList = BehaviorSubject();
  Stream<String> get dataList => _dataList.stream;

  Stream<bool> get button => _button.stream;

  final BehaviorSubject<User> _login = BehaviorSubject();
  Stream<User> get login => _login.stream;

  final FirebaseManager _firebaseManager = FirebaseManager();

  LoginViewModel() {
    _button.addStream(Rx.combineLatest2(_password.stream, _email.stream,
        (a, b) => b.length > 2 && a.length > 5));
  }

  void onPassword(String password) {
    _password.sink.add(password);
  }

  void onEmailChanged(String email) {
    _email.sink.add(email);
  }

  void submit() {
    final email = _email.value;
    final password = _password.value;

    _firebaseManager.validate(email: email, password: password).listen((value) {
      _login.sink.add(value);
    }).onError((error) {
      _login.sink.addError("Login Failed");
    });
  }

  void fetchData() {
    _firebaseManager.getBoilerMsg().listen((event) {
      _dataList.sink.add(event);
    });
  }

  @override
  void dispose() {
    // TODO: implement dispose
  }
}
